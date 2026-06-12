// session.cpp
#include "session.hpp"
#include "logger.hpp"

// Network
#include "network/signaling_client.hpp"
#include "network/ice_transport.hpp"
#include "network/media_channel.hpp"
#include "network/bitrate_controller.hpp"

// Crypto
#include "crypto/crypto_session.hpp"

// Capture
#include "capture/dxgi_capture.hpp"

// Codec
#include "codec/nvenc_encoder.hpp"
#include "codec/ffmpeg_encoder.hpp"
#include "codec/ffmpeg_decoder.hpp"

// Input
#include "input/input_capture_win32.hpp"
#include "input/input_injector_win32.hpp"
#include "input/clipboard_sync.hpp"

#include <d3d11.h>
#include <QTimer>

namespace rapiddesk::core {

    Session::Session(QObject* parent) : QObject(parent) {
        // Initialize D3D11 device for capture + encoder
        D3D_FEATURE_LEVEL feature_level;
        D3D11CreateDevice(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
            D3D11_CREATE_DEVICE_BGRA_SUPPORT,
            nullptr, 0, D3D11_SDK_VERSION,
            reinterpret_cast<ID3D11Device**>(&d3d_device_),
            &feature_level,
            nullptr);
    }

    Session::~Session() {
        shutdown();
        if (d3d_device_) {
            reinterpret_cast<ID3D11Device*>(d3d_device_)->Release();
        }
    }

    void Session::initialize_host() {
        is_host_ = true;

        // Generate session ID
        session_id_ = generate_session_id(); // 9-digit ID
        emit session_id_ready(QString::fromStdString(session_id_));

        // Initialize crypto
        crypto_ = std::make_unique<crypto::CryptoSession>();
        crypto_->generate_ephemeral_keypair();

        // Setup capture
        capture_ = std::make_unique<capture::DXGICapture>();
        if (!capture_->initialize(reinterpret_cast<ID3D11Device*>(d3d_device_))) {
            emit connection_error("Falha ao inicializar captura de tela");
            return;
        }

        // Try hardware encoder first
        hw_encoder_ = std::make_unique<codec::NVEncEncoder>();
        if (hw_encoder_->initialize(reinterpret_cast<ID3D11Device*>(d3d_device_),
            capture_->width(), capture_->height())) {
            hw_encoder_->set_callback([this](const auto& frame) { on_encoded_frame(frame); });
            capture_->set_frame_callback([this](auto* tex) { hw_encoder_->encode_texture(tex); });
        }
        else {
            // Fallback to software encoder
            hw_encoder_.reset();
            sw_encoder_ = std::make_unique<codec::FFmpegEncoder>();
            if (!sw_encoder_->initialize(capture_->width(), capture_->height())) {
                emit connection_error("Falha ao inicializar encoder");
                return;
            }
            sw_encoder_->set_callback([this](const auto& frame) { on_encoded_frame(frame); });
            // Would need RGBA capture path for software encoder
        }

        // Setup input injection (host receives input from viewer)
        input_injector_ = std::make_unique<input::InputInjectorWin32>();
        input_injector_->initialize();

        // Setup clipboard
        clipboard_ = std::make_unique<input::ClipboardSync>();
        clipboard_->initialize(true); // is_host = true
        clipboard_->set_send_callback([this](const auto& msg) {
            // Send via media channel
            });

        // Initialize network
        setup_host_pipeline();
    }

    void Session::initialize_viewer(const std::string& session_id, const std::string& password) {
        is_host_ = false;
        session_id_ = session_id;

        // Initialize crypto
        crypto_ = std::make_unique<crypto::CryptoSession>();
        crypto_->generate_ephemeral_keypair();

        // Setup decoder
        decoder_ = std::make_unique<codec::FFmpegDecoder>();
        // Will initialize with remote resolution after connection

        // Setup input capture (viewer captures local input)
        input_capture_ = std::make_unique<input::InputCaptureWin32>();
        // Need HWND from UI — will be set later
        // input_capture_->initialize(hwnd);

        // Setup clipboard
        clipboard_ = std::make_unique<input::ClipboardSync>();
        clipboard_->initialize(false); // is_host = false

        setup_viewer_pipeline();
    }

    void Session::setup_host_pipeline() {
        signaling_ = std::make_unique<network::SignalingClient>();
        ice_ = std::make_unique<network::ICETransport>();
        media_ = std::make_unique<network::MediaChannel>();
        bitrate_ctrl_ = std::make_unique<network::BitrateController>();

        // Connect signaling → ICE
        connect(signaling_.get(), &network::SignalingClient::connected,
            this, &Session::on_signaling_connected);

        // Connect ICE → Media
        ice_->set_on_connected([this]() { on_ice_connected(); });

        // Connect Media → Crypto → Encoder/Decoder
        media_->set_receive_callback([this](const auto& data) { on_media_packet_received(data); });

        // Connect bitrate controller → encoder
        bitrate_ctrl_->set_on_bitrate_changed([this](uint32_t bitrate) {
            if (hw_encoder_) hw_encoder_->update_bitrate(bitrate);
            else if (sw_encoder_) sw_encoder_->update_bitrate(bitrate);
            });

        // Start signaling connection
        signaling_->connect_to_server("wss://signaling.rapiddesk.internal");
    }

    void Session::setup_viewer_pipeline() {
        signaling_ = std::make_unique<network::SignalingClient>();
        ice_ = std::make_unique<network::ICETransport>();
        media_ = std::make_unique<network::MediaChannel>();

        // Similar connections as host but reversed data flow
        // ...

        signaling_->connect_to_server("wss://signaling.rapiddesk.internal");
        signaling_->request_session(session_id_);
    }

    void Session::on_signaling_connected() {
        // Exchange ICE candidates
        auto offer = ice_->create_offer();
        signaling_->send_ice_offer(session_id_, offer, crypto_->public_key());
    }

    void Session::on_ice_connected() {
        connected_ = true;
        emit connection_established();

        // Start capture loop (host) or render loop (viewer)
        if (is_host_) {
            capture_->start();
        }
    }

    void Session::on_encoded_frame(const codec::EncodedFrame& frame) {
        if (!media_ || !connected_) return;

        // Encrypt and send
        auto encrypted = crypto_->encrypt(frame.data);
        media_->send_video_packet(encrypted, frame.timestamp_us, frame.is_keyframe);
    }

    void Session::on_media_packet_received(const std::vector<uint8_t>& data) {
        if (!crypto_) return;

        auto decrypted = crypto_->decrypt(data);
        if (!decrypted) {
            LOG_WARN("Failed to decrypt packet");
            return;
        }

        if (is_host_) {
            // Host receives input or clipboard
            // Parse input packet and inject
        }
        else {
            // Viewer receives video
            if (!decoder_->is_initialized()) {
                // Need to initialize decoder with SPS/PPS from first keyframe
                // or negotiate resolution during handshake
            }
            decoder_->decode(decrypted->data(), decrypted->size(), 0);
        }
    }

    void Session::on_decoded_frame(const codec::DecodedFrame& frame) {
        emit frame_received(
            std::vector<uint8_t>(frame.rgba_data.begin(), frame.rgba_data.end()),
            frame.width, frame.height);
    }

    void Session::on_input_event(const input::InputEvent& event) {
        if (!media_ || !connected_) return;

        // Serialize and encrypt input event
        // Send via media channel (high priority stream)
    }

    void Session::shutdown() {
        connected_ = false;

        if (capture_) capture_->stop();
        if (input_capture_) input_capture_->shutdown();
        if (input_injector_) input_injector_->shutdown();
        if (clipboard_) clipboard_->shutdown();

        media_.reset();
        ice_.reset();
        signaling_.reset();
    }

    std::string Session::generate_session_id() {
        // 9-digit numeric ID for easy sharing
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(100000000, 999999999);
        return std::to_string(dis(gen));
    }

} // namespace rapiddesk::core
// session.hpp
#pragma once

#include <QObject>
#include <memory>
#include <string>
#include <functional>

namespace rapiddesk::network {
    class SignalingClient;
    class ICETransport;
    class MediaChannel;
    class BitrateController;
}

namespace rapiddesk::crypto {
    class CryptoSession;
}

namespace rapiddesk::capture {
    class DXGICapture;
}

namespace rapiddesk::codec {
    class NVEncEncoder;
    class FFmpegEncoder;
    class FFmpegDecoder;
}

namespace rapiddesk::input {
    class InputCaptureWin32;
    class InputInjectorWin32;
    class ClipboardSync;
}

namespace rapiddesk::core {

    class Session : public QObject {
        Q_OBJECT

    public:
        explicit Session(QObject* parent = nullptr);
        ~Session() override;

        // Host mode
        void initialize_host();

        // Viewer mode
        void initialize_viewer(const std::string& session_id, const std::string& password);

        void shutdown();

        bool is_host() const noexcept { return is_host_; }
        bool is_connected() const noexcept { return connected_; }

        // Stats
        double glass_to_glass_latency_ms() const;
        double input_latency_ms() const;

    signals:
        void session_id_ready(const QString& id);
        void connection_established();
        void connection_error(const QString& error);
        void frame_received(const std::vector<uint8_t>& rgba_data, uint32_t width, uint32_t height);
        void latency_updated(double glass_ms, double input_ms);

    private:
        void setup_host_pipeline();
        void setup_viewer_pipeline();
        void on_signaling_connected();
        void on_ice_connected();
        void on_media_packet_received(const std::vector<uint8_t>& data);
        void on_capture_frame(ID3D11Texture2D* texture);
        void on_encoded_frame(const codec::EncodedFrame& frame);
        void on_decoded_frame(const codec::DecodedFrame& frame);
        void on_input_event(const input::InputEvent& event);
        void on_clipboard_message(const input::ClipboardMessage& msg);

        // Components
        std::unique_ptr<network::SignalingClient> signaling_;
        std::unique_ptr<network::ICETransport> ice_;
        std::unique_ptr<network::MediaChannel> media_;
        std::unique_ptr<network::BitrateController> bitrate_ctrl_;

        std::unique_ptr<crypto::CryptoSession> crypto_;

        // Host-only
        std::unique_ptr<capture::DXGICapture> capture_;
        std::unique_ptr<codec::NVEncEncoder> hw_encoder_;
        std::unique_ptr<codec::FFmpegEncoder> sw_encoder_;

        // Viewer-only
        std::unique_ptr<codec::FFmpegDecoder> decoder_;

        // Input
        std::unique_ptr<input::InputCaptureWin32> input_capture_;
        std::unique_ptr<input::InputInjectorWin32> input_injector_;
        std::unique_ptr<input::ClipboardSync> clipboard_;

        bool is_host_ = false;
        bool connected_ = false;
        std::string session_id_;

        // D3D11 device (shared between capture and encoder)
        void* d3d_device_ = nullptr;  // ID3D11Device*
    };

} // namespace rapiddesk::core
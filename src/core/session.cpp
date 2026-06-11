// src/core/session.cpp
#include "session.hpp"
#include "../network/media_channel.hpp"
#include "../crypto/crypto_session.hpp"
#include "../capture/dxgi_capture.hpp"
#include "../codec/nvenc_encoder.hpp"
#include "../network/bitrate_controller.hpp"
#include <thread>

void Session::run_host_session_loop(std::atomic<bool>& running) {
    PacketPool pool;
    CryptoSession crypto;
    DXGICaptureMock capture;
    NVENCEncoderMock encoder;
    BitrateController bitrate_manager;
    // ... Todo o resto do laço while(running) que estruturamos vai aqui ...
}
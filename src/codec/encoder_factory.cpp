// encoder_factory.cpp
#include "encoder_factory.hpp"
#include "nvenc_encoder.hpp"
#include "ffmpeg_encoder.hpp"

namespace rapiddesk::codec {

    std::unique_ptr<NVEncEncoder> EncoderFactory::create_hw_encoder() {
        auto encoder = std::make_unique<NVEncEncoder>();
        // Inicialização com D3D11 device passado externamente
        return encoder;
    }

    std::unique_ptr<FFmpegEncoder> EncoderFactory::create_sw_encoder() {
        auto encoder = std::make_unique<FFmpegEncoder>();
        return encoder;
    }

} // namespace rapiddesk::codec
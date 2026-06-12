// encoder_factory.hpp
#pragma once
#include <memory>

namespace rapiddesk::codec {

    class NVEncEncoder;
    class FFmpegEncoder;

    class EncoderFactory {
    public:
        // Tenta NVENC primeiro, fallback para FFmpeg CPU
        static std::unique_ptr<NVEncEncoder> create_hw_encoder();
        static std::unique_ptr<FFmpegEncoder> create_sw_encoder();
    };

} // namespace rapiddesk::codec
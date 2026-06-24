#include "codec/ffmpeg_encoder.hpp"

namespace rapiddesk::codec {

    class FFmpegEncoder::Impl {};

    FFmpegEncoder::FFmpegEncoder() = default;
    FFmpegEncoder::~FFmpegEncoder() = default;

    bool FFmpegEncoder::initialize(int width, int height, int fps, int bitrate) {
        (void)width; (void)height; (void)fps; (void)bitrate;
        return false;  // FFmpeg não disponível
    }

    void FFmpegEncoder::set_callback(EncodeCallback cb) {
        callback_ = std::move(cb);
    }

    bool FFmpegEncoder::encode_frame(const uint8_t* rgba_data, int width, int height) {
        (void)rgba_data; (void)width; (void)height;
        return false;
    }

    void FFmpegEncoder::shutdown() {}

} // namespace rapiddesk::codec
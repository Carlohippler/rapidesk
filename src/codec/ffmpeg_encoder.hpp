#pragma once
#include "codec/frame_types.hpp"
#include <functional>

namespace rapiddesk::codec {

    class FFmpegEncoder {
    public:
        FFmpegEncoder();
        ~FFmpegEncoder();

        bool initialize(int width, int height, int fps, int bitrate);
        void set_callback(EncodeCallback cb);
        bool encode_frame(const uint8_t* rgba_data, int width, int height);
        void shutdown();

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;
        EncodeCallback callback_;
    };

} // namespace rapiddesk::codec
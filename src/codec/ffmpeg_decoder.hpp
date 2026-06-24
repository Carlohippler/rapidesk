#pragma once
#include "codec/frame_types.hpp"
#include <functional>

namespace rapiddesk::codec {

    class FFmpegDecoder {
    public:
        FFmpegDecoder();
        ~FFmpegDecoder();

        bool initialize(int width, int height);
        void set_callback(DecodeCallback cb);
        bool decode(const uint8_t* encoded_data, size_t size);
        void shutdown();

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;
        DecodeCallback callback_;
    };

} // namespace rapiddesk::codec
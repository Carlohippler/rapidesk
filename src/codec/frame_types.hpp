#pragma once
#include <vector>
#include <cstdint>

namespace rapiddesk::codec {

    struct EncodedFrame {
        std::vector<uint8_t> data;
        int64_t timestamp = 0;
        bool keyframe = false;
        int width = 0;
        int height = 0;
    };

    struct DecodedFrame {
        std::vector<uint8_t> rgba_data;  // pixels RGBA
        int width = 0;
        int height = 0;
        int64_t timestamp = 0;
    };

    using EncodeCallback = std::function<void(const EncodedFrame&)>;
    using DecodeCallback = std::function<void(const DecodedFrame&)>;

} // namespace rapiddesk::codec
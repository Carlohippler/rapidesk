#include <gtest/gtest.h>
#include "codec/ffmpeg_encoder.hpp"
#include "codec/ffmpeg_decoder.hpp"
#include <vector>
#include <cstring>

using namespace rapiddesk::codec;

class CodecTest : public ::testing::Test {
protected:
    static constexpr uint32_t WIDTH = 320;
    static constexpr uint32_t HEIGHT = 240;

    std::vector<uint8_t> generate_test_frame() {
        std::vector<uint8_t> frame(WIDTH * HEIGHT * 4);
        // Gradient pattern
        for (uint32_t y = 0; y < HEIGHT; ++y) {
            for (uint32_t x = 0; x < WIDTH; ++x) {
                size_t idx = (y * WIDTH + x) * 4;
                frame[idx + 0] = static_cast<uint8_t>(x % 256);     // B
                frame[idx + 1] = static_cast<uint8_t>(y % 256);     // G
                frame[idx + 2] = static_cast<uint8_t>((x + y) % 256); // R
                frame[idx + 3] = 0xFF;                             // A
            }
        }
        return frame;
    }
};

TEST_F(CodecTest, FFmpegEncoderInitialization) {
    FFmpegEncoder encoder;
    EXPECT_TRUE(encoder.initialize(WIDTH, HEIGHT, 30, 1'000'000));
    EXPECT_TRUE(encoder.is_initialized());
}

TEST_F(CodecTest, FFmpegEncoderProducesOutput) {
    FFmpegEncoder encoder;
    ASSERT_TRUE(encoder.initialize(WIDTH, HEIGHT, 30, 1'000'000));

    std::vector<uint8_t> output;
    encoder.set_callback([&](const EncodedFrame& frame) {
        output.assign(frame.data.begin(), frame.data.end());
        });

    auto frame = generate_test_frame();
    encoder.encode_rgba(frame.data(), 0, true); // Force keyframe

    // Give encoder time to process
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    EXPECT_FALSE(output.empty());
    // H.264 start code or NAL prefix
    EXPECT_TRUE(output[0] == 0x00 && output[1] == 0x00);
}

TEST_F(CodecTest, FFmpegDecoderInitialization) {
    FFmpegDecoder decoder;
    EXPECT_TRUE(decoder.initialize(WIDTH, HEIGHT, false)); // Software only for test
    EXPECT_TRUE(decoder.is_initialized());
}

TEST_F(CodecTest, EncodeDecodeRoundtrip) {
    FFmpegEncoder encoder;
    FFmpegDecoder decoder;

    ASSERT_TRUE(encoder.initialize(WIDTH, HEIGHT, 30, 2'000'000));
    ASSERT_TRUE(decoder.initialize(WIDTH, HEIGHT, false));

    std::vector<uint8_t> encoded_data;
    encoder.set_callback([&](const EncodedFrame& frame) {
        encoded_data.assign(frame.data.begin(), frame.data.end());
        });

    DecodedFrame decoded_frame;
    bool decoded = false;
    decoder.set_callback([&](const DecodedFrame& frame) {
        decoded_frame = frame;
        decoded = true;
        });

    // Encode
    auto input = generate_test_frame();
    encoder.encode_rgba(input.data(), 0, true);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Decode
    ASSERT_FALSE(encoded_data.empty());
    decoder.decode(encoded_data.data(), encoded_data.size(), 0);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    EXPECT_TRUE(decoded);
    EXPECT_EQ(decoded_frame.width, WIDTH);
    EXPECT_EQ(decoded_frame.height, HEIGHT);
    EXPECT_FALSE(decoded_frame.rgba_data.empty());
}

TEST_F(CodecTest, KeyframeForced) {
    FFmpegEncoder encoder;
    ASSERT_TRUE(encoder.initialize(WIDTH, HEIGHT, 30, 1'000'000));

    bool got_keyframe = false;
    encoder.set_callback([&](const EncodedFrame& frame) {
        if (frame.is_keyframe) got_keyframe = true;
        });

    auto frame = generate_test_frame();
    encoder.force_idr();
    encoder.encode_rgba(frame.data(), 0);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_TRUE(got_keyframe);
}

TEST_F(CodecTest, BitrateUpdate) {
    FFmpegEncoder encoder;
    ASSERT_TRUE(encoder.initialize(WIDTH, HEIGHT, 30, 1'000'000));

    encoder.update_bitrate(500'000);
    // Should not crash — actual bitrate verification requires parsing SPS
    SUCCEED();
}

TEST_F(CodecTest, DecoderFlush) {
    FFmpegDecoder decoder;
    ASSERT_TRUE(decoder.initialize(WIDTH, HEIGHT, false));

    // Flush should not crash even with no data
    decoder.flush();
    SUCCEED();
}
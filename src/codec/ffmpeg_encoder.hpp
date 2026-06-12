#pragma once

#include <cstdint>
#include <memory>
#include <span>
#include <functional>

struct AVCodecContext;
struct AVFrame;
struct AVPacket;
struct SwsContext;

namespace rapiddesk::codec {

    struct EncodedFrame;
    using EncodeCallback = std::function<void(const EncodedFrame&)>;

    /**
     * FFmpeg software encoder fallback (libx264).
     * Configuration: ultrafast preset, zerolatency tune, baseline profile.
     * Used when no hardware encoder is available.
     */
    class FFmpegEncoder {
    public:
        FFmpegEncoder();
        ~FFmpegEncoder();

        FFmpegEncoder(const FFmpegEncoder&) = delete;
        FFmpegEncoder& operator=(const FFmpegEncoder&) = delete;

        bool initialize(uint32_t width,
            uint32_t height,
            uint32_t fps = 60,
            uint32_t bitrate_bps = 2'000'000);

        /**
         * Encode an RGBA frame from CPU memory.
         * @param rgba_data RGBA pixel data, width*height*4 bytes
         * @param timestamp_us Capture timestamp
         * @param force_keyframe Force IDR
         */
        void encode_rgba(const uint8_t* rgba_data,
            uint64_t timestamp_us = 0,
            bool force_keyframe = false);

        void update_bitrate(uint32_t bitrate_bps);
        void force_idr();
        void set_callback(EncodeCallback cb);

        uint32_t width() const noexcept { return width_; }
        uint32_t height() const noexcept { return height_; }
        bool is_initialized() const noexcept { return codec_ctx_ != nullptr; }

    private:
        AVCodecContext* codec_ctx_ = nullptr;
        AVFrame* frame_ = nullptr;
        AVPacket* packet_ = nullptr;
        SwsContext* sws_ctx_ = nullptr;

        uint32_t width_ = 0;
        uint32_t height_ = 0;
        uint32_t fps_ = 60;
        uint32_t target_bitrate_ = 2'000'000;
        int64_t frame_count_ = 0;

        EncodeCallback callback_;
        bool pending_idr_ = false;

        bool send_frame(AVFrame* frame, uint64_t timestamp_us, bool force_keyframe);
        void receive_packets(uint64_t timestamp_us);
    };

} // namespace rapiddesk::codec
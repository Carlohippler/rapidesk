#pragma once

#include <cstdint>
#include <memory>
#include <span>
#include <functional>

struct AVCodecContext;
struct AVFrame;
struct AVPacket;
struct AVBufferRef;
struct SwsContext;

namespace rapiddesk::codec {

    struct DecodedFrame {
        std::span<const uint8_t> rgba_data;  // width * height * 4
        uint32_t width;
        uint32_t height;
        uint64_t timestamp_us;
        bool is_keyframe;
    };

    using DecodeCallback = std::function<void(const DecodedFrame&)>;

    /**
     * FFmpeg H.264 decoder -> RGBA output for viewer rendering.
     * Supports hardware acceleration via DXVA2/D3D11VA (Windows),
     * VAAPI (Linux), or VideoToolbox (macOS) when available.
     */
    class FFmpegDecoder {
    public:
        FFmpegDecoder();
        ~FFmpegDecoder();

        FFmpegDecoder(const FFmpegDecoder&) = delete;
        FFmpegDecoder& operator=(const FFmpegDecoder&) = delete;

        bool initialize(uint32_t width, uint32_t height, bool use_hw_accel = true);

        /**
         * Decode H.264 NAL unit(s).
         * @param h264_data H.264 encoded data (can be partial frame)
         * @param timestamp_us Presentation timestamp
         */
        void decode(const uint8_t* h264_data, size_t len, uint64_t timestamp_us);

        void set_callback(DecodeCallback cb);

        uint32_t width() const noexcept { return width_; }
        uint32_t height() const noexcept { return height_; }
        bool is_initialized() const noexcept { return codec_ctx_ != nullptr; }

        /** Flush decoder (e.g. on resolution change). */
        void flush();

    private:
        AVCodecContext* codec_ctx_ = nullptr;
        AVFrame* hw_frame_ = nullptr;    // Hardware decoded frame
        AVFrame* sw_frame_ = nullptr;    // Software/RGBA frame
        AVPacket* packet_ = nullptr;
        AVBufferRef* hw_device_ctx_ = nullptr;
        SwsContext* sws_ctx_ = nullptr;

        uint32_t width_ = 0;
        uint32_t height_ = 0;
        bool hw_accel_enabled_ = false;

        DecodeCallback callback_;
        uint8_t* rgba_buffer_ = nullptr;
        size_t rgba_buffer_size_ = 0;

        bool init_hw_accel();
        void process_frame(AVFrame* frame, uint64_t timestamp_us);
        bool ensure_rgba_buffer(size_t size);
    };

} // namespace rapiddesk::codec
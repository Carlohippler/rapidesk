#include "ffmpeg_decoder.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libavutil/hwcontext.h>
#include <libswscale/swscale.h>
}

#ifdef _WIN32
#define HW_ACCEL_TYPE AV_HWDEVICE_TYPE_DXVA2
#elif __linux__
#define HW_ACCEL_TYPE AV_HWDEVICE_TYPE_VAAPI
#elif __APPLE__
#define HW_ACCEL_TYPE AV_HWDEVICE_TYPE_VIDEOTOOLBOX
#else
#define HW_ACCEL_TYPE AV_HWDEVICE_TYPE_NONE
#endif

namespace rapiddesk::codec {

    FFmpegDecoder::FFmpegDecoder() = default;

    FFmpegDecoder::~FFmpegDecoder() {
        if (rgba_buffer_) av_free(rgba_buffer_);
        if (sws_ctx_) sws_freeContext(sws_ctx_);
        if (packet_) av_packet_free(&packet_);
        if (hw_frame_) av_frame_free(&hw_frame_);
        if (sw_frame_) av_frame_free(&sw_frame_);
        if (codec_ctx_) avcodec_free_context(&codec_ctx_);
        if (hw_device_ctx_) av_buffer_unref(&hw_device_ctx_);
    }

    bool FFmpegDecoder::initialize(uint32_t width, uint32_t height, bool use_hw_accel) {
        width_ = width;
        height_ = height;

        const AVCodec* codec = avcodec_find_decoder(AV_CODEC_ID_H264);
        if (!codec) return false;

        codec_ctx_ = avcodec_alloc_context3(codec);
        if (!codec_ctx_) return false;

        codec_ctx_->width = static_cast<int>(width_);
        codec_ctx_->height = static_cast<int>(height_);
        codec_ctx_->pix_fmt = AV_PIX_FMT_YUV420P;
        codec_ctx_->thread_count = 4;
        codec_ctx_->flags |= AV_CODEC_FLAG_LOW_DELAY;

        // Enable hardware acceleration
        if (use_hw_accel && HW_ACCEL_TYPE != AV_HWDEVICE_TYPE_NONE) {
            hw_accel_enabled_ = init_hw_accel();
        }

        int ret = avcodec_open2(codec_ctx_, codec, nullptr);
        if (ret < 0) {
            avcodec_free_context(&codec_ctx_);
            return false;
        }

        packet_ = av_packet_alloc();
        sw_frame_ = av_frame_alloc();
        if (hw_accel_enabled_) {
            hw_frame_ = av_frame_alloc();
        }

        // Pre-allocate RGBA buffer
        rgba_buffer_size_ = width_ * height_ * 4;
        rgba_buffer_ = static_cast<uint8_t*>(av_malloc(rgba_buffer_size_ + AV_INPUT_BUFFER_PADDING_SIZE));

        return true;
    }

    bool FFmpegDecoder::init_hw_accel() {
        int ret = av_hwdevice_ctx_create(&hw_device_ctx_, HW_ACCEL_TYPE, nullptr, nullptr, 0);
        if (ret < 0) return false;

        codec_ctx_->hw_device_ctx = av_buffer_ref(hw_device_ctx_);
        codec_ctx_->get_format = [](AVCodecContext* ctx, const enum AVPixelFormat* pix_fmts) -> AVPixelFormat {
            for (const AVPixelFormat* p = pix_fmts; *p != AV_PIX_FMT_NONE; p++) {
                if (*p == AV_PIX_FMT_DXVA2_VLD || *p == AV_PIX_FMT_VAAPI ||
                    *p == AV_PIX_FMT_VIDEOTOOLBOX) {
                    return *p;
                }
            }
            return pix_fmts[0]; // Fallback to software
            };
        return true;
    }

    void FFmpegDecoder::decode(const uint8_t* h264_data, size_t len, uint64_t timestamp_us) {
        if (!codec_ctx_ || !packet_) return;

        packet_->data = const_cast<uint8_t*>(h264_data);
        packet_->size = static_cast<int>(len);

        int ret = avcodec_send_packet(codec_ctx_, packet_);
        if (ret < 0 && ret != AVERROR(EAGAIN)) {
            // Decode error, try to recover
            return;
        }

        AVFrame* frame = hw_accel_enabled_ ? hw_frame_ : sw_frame_;

        while ((ret = avcodec_receive_frame(codec_ctx_, frame)) == 0) {
            if (hw_accel_enabled_ && frame->format != AV_PIX_FMT_YUV420P) {
                // Transfer from hardware to software frame
                ret = av_hwframe_transfer_data(sw_frame_, frame, 0);
                if (ret < 0) continue;
                av_frame_copy_props(sw_frame_, frame);
                process_frame(sw_frame_, timestamp_us);
            }
            else {
                process_frame(frame, timestamp_us);
            }
        }

        av_packet_unref(packet_);
    }

    void FFmpegDecoder::process_frame(AVFrame* frame, uint64_t timestamp_us) {
        if (!frame || !frame->data[0]) return;

        // Ensure RGBA buffer is large enough
        if (!ensure_rgba_buffer(width_ * height_ * 4)) return;

        // Convert YUV420P to RGBA
        if (!sws_ctx_ ||
            sws_getCachedContext(&sws_ctx_,
                frame->width, frame->height, static_cast<AVPixelFormat>(frame->format),
                static_cast<int>(width_), static_cast<int>(height_), AV_PIX_FMT_BGRA,
                SWS_FAST_BILINEAR, nullptr, nullptr, nullptr) == nullptr) {
            return;
        }

        uint8_t* dst_data[1] = { rgba_buffer_ };
        int dst_linesize[1] = { static_cast<int>(width_ * 4) };

        sws_scale(sws_ctx_, frame->data, frame->linesize, 0,
            frame->height, dst_data, dst_linesize);

        if (callback_) {
            DecodedFrame out;
            out.rgba_data = std::span<const uint8_t>(rgba_buffer_, width_ * height_ * 4);
            out.width = width_;
            out.height = height_;
            out.timestamp_us = timestamp_us;
            out.is_keyframe = (frame->pict_type == AV_PICTURE_TYPE_I) || frame->key_frame;
            callback_(out);
        }
    }

    bool FFmpegDecoder::ensure_rgba_buffer(size_t size) {
        if (rgba_buffer_size_ >= size) return true;

        if (rgba_buffer_) av_free(rgba_buffer_);
        rgba_buffer_size_ = size;
        rgba_buffer_ = static_cast<uint8_t*>(av_malloc(rgba_buffer_size_ + AV_INPUT_BUFFER_PADDING_SIZE));
        return rgba_buffer_ != nullptr;
    }

    void FFmpegDecoder::set_callback(DecodeCallback cb) {
        callback_ = std::move(cb);
    }

    void FFmpegDecoder::flush() {
        if (codec_ctx_) {
            avcodec_flush_buffers(codec_ctx_);
        }
    }

} // namespace rapiddesk::codec
#include "ffmpeg_encoder.hpp"
#include "nvenc_encoder.hpp"  // For EncodedFrame definition

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

namespace rapiddesk::codec {

    FFmpegEncoder::FFmpegEncoder() = default;

    FFmpegEncoder::~FFmpegEncoder() {
        if (sws_ctx_) sws_freeContext(sws_ctx_);
        if (packet_) av_packet_free(&packet_);
        if (frame_) av_frame_free(&frame_);
        if (codec_ctx_) {
            avcodec_free_context(&codec_ctx_);
        }
    }

    bool FFmpegEncoder::initialize(uint32_t width,
        uint32_t height,
        uint32_t fps,
        uint32_t bitrate_bps) {
        width_ = width;
        height_ = height;
        fps_ = fps;
        target_bitrate_ = bitrate_bps;

        const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_H264);
        if (!codec) return false;

        codec_ctx_ = avcodec_alloc_context3(codec);
        if (!codec_ctx_) return false;

        // Low-latency configuration
        codec_ctx_->width = static_cast<int>(width_);
        codec_ctx_->height = static_cast<int>(height_);
        codec_ctx_->time_base = AVRational{ 1, static_cast<int>(fps_) };
        codec_ctx_->framerate = AVRational{ static_cast<int>(fps_), 1 };
        codec_ctx_->pix_fmt = AV_PIX_FMT_YUV420P;
        codec_ctx_->bit_rate = static_cast<int64_t>(target_bitrate_);
        codec_ctx_->rc_max_rate = static_cast<int64_t>(target_bitrate_ * 1.5);
        codec_ctx_->rc_buffer_size = static_cast<int>(target_bitrate_ / 4); // 250ms
        codec_ctx_->gop_size = 9999;  // Infinite GOP, IDR on demand
        codec_ctx_->max_b_frames = 0; // No B-frames
        codec_ctx_->thread_count = 4; // Multi-threaded encoding

        // x264 specific options for minimal latency
        av_opt_set(codec_ctx_->priv_data, "preset", "ultrafast", 0);
        av_opt_set(codec_ctx_->priv_data, "tune", "zerolatency", 0);
        av_opt_set(codec_ctx_->priv_data, "profile", "baseline", 0);
        av_opt_set(codec_ctx_->priv_data, "intra-refresh", "0", 0);

        int ret = avcodec_open2(codec_ctx_, codec, nullptr);
        if (ret < 0) {
            avcodec_free_context(&codec_ctx_);
            return false;
        }

        // Allocate frame
        frame_ = av_frame_alloc();
        frame_->format = AV_PIX_FMT_YUV420P;
        frame_->width = codec_ctx_->width;
        frame_->height = codec_ctx_->height;
        ret = av_frame_get_buffer(frame_, 0);
        if (ret < 0) return false;

        packet_ = av_packet_alloc();

        // Initialize RGBA -> YUV420P converter
        sws_ctx_ = sws_getContext(
            static_cast<int>(width_), static_cast<int>(height_), AV_PIX_FMT_BGRA,
            codec_ctx_->width, codec_ctx_->height, AV_PIX_FMT_YUV420P,
            SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);

        return sws_ctx_ != nullptr;
    }

    void FFmpegEncoder::encode_rgba(const uint8_t* rgba_data,
        uint64_t timestamp_us,
        bool force_keyframe) {
        if (!codec_ctx_ || !frame_ || !rgba_data) return;

        // Convert BGRA to YUV420P
        const uint8_t* src_data[1] = { rgba_data };
        int src_linesize[1] = { static_cast<int>(width_ * 4) };
        sws_scale(sws_ctx_, src_data, src_linesize, 0,
            static_cast<int>(height_), frame_->data, frame_->linesize);

        frame_->pts = frame_count_++;

        send_frame(frame_, timestamp_us, force_keyframe);
    }

    bool FFmpegEncoder::send_frame(AVFrame* frame, uint64_t timestamp_us, bool force_keyframe) {
        if (force_keyframe || pending_idr_) {
            frame->pict_type = AV_PICTURE_TYPE_I;
            frame->key_frame = 1;
            pending_idr_ = false;
        }
        else {
            frame->pict_type = AV_PICTURE_TYPE_NONE; // Let encoder decide
        }

        int ret = avcodec_send_frame(codec_ctx_, frame);
        if (ret < 0 && ret != AVERROR(EAGAIN)) return false;

        receive_packets(timestamp_us);
        return true;
    }

    void FFmpegEncoder::receive_packets(uint64_t timestamp_us) {
        int ret;
        while ((ret = avcodec_receive_packet(codec_ctx_, packet_)) == 0) {
            if (callback_) {
                EncodedFrame frame;
                frame.data = std::span<const uint8_t>(packet_->data, packet_->size);
                frame.timestamp_us = timestamp_us;
                frame.is_keyframe = (packet_->flags & AV_PKT_FLAG_KEY) != 0;
                frame.width = width_;
                frame.height = height_;
                callback_(frame);
            }
            av_packet_unref(packet_);
        }
    }

    void FFmpegEncoder::update_bitrate(uint32_t bitrate_bps) {
        target_bitrate_ = bitrate_bps;
        if (codec_ctx_) {
            codec_ctx_->bit_rate = static_cast<int64_t>(target_bitrate_);
            // Note: Some encoders support dynamic bitrate via side data or reinit
        }
    }

    void FFmpegEncoder::force_idr() {
        pending_idr_ = true;
    }

    void FFmpegEncoder::set_callback(EncodeCallback cb) {
        callback_ = std::move(cb);
    }

} // namespace rapiddesk::codec
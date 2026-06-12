#pragma once

#include <cstdint>
#include <memory>
#include <array>
#include <span>
#include <functional>
#include <wrl/client.h>  // Microsoft::WRL::ComPtr

// Forward declarations
struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11Texture2D;
struct NV_ENC_REGISTERED_PTR;
struct NV_ENC_OUTPUT_PTR;

namespace rapiddesk::codec {

    struct EncodedFrame {
        std::span<const uint8_t> data;
        uint64_t timestamp_us;
        bool is_keyframe;
        uint32_t width;
        uint32_t height;
    };

    using EncodeCallback = std::function<void(const EncodedFrame&)>;

    /**
     * NVENC hardware encoder — zero-copy path from D3D11 texture.
     * Configured for minimum glass-to-glass latency:
     *   - P1 preset (fastest)
     *   - Zero B-frames
     *   - CBR low-delay HQ
     *   - VBV buffer = 250ms
     *   - Infinite GOP (IDR on demand only)
     */
    class NVEncEncoder {
    public:
        NVEncEncoder();
        ~NVEncEncoder();

        // Non-copyable, non-movable (contains raw handles)
        NVEncEncoder(const NVEncEncoder&) = delete;
        NVEncEncoder& operator=(const NVEncEncoder&) = delete;

        /**
         * Initialize encoder with D3D11 device.
         * @param d3d_device D3D11 device for zero-copy texture input
         * @param width Frame width (must be aligned to 16)
         * @param height Frame height (must be aligned to 16)
         * @param bitrate_bps Target bitrate in bits/sec (default 4 Mbps)
         * @return true on success
         */
        bool initialize(ID3D11Device* d3d_device,
            uint32_t width,
            uint32_t height,
            uint32_t bitrate_bps = 4'000'000);

        /**
         * Encode a frame directly from D3D11 texture (zero-copy).
         * @param texture GPU texture containing the frame
         * @param force_keyframe Force IDR frame (e.g. on scene change)
         * @param timestamp_us Capture timestamp for A/V sync
         */
        void encode_texture(ID3D11Texture2D* texture,
            bool force_keyframe = false,
            uint64_t timestamp_us = 0);

        /** Update target bitrate dynamically (adaptive bitrate). */
        void update_bitrate(uint32_t bitrate_bps);

        /** Request an IDR frame at next encode. */
        void force_idr();

        /** Set callback for encoded bitstream output. */
        void set_callback(EncodeCallback cb);

        uint32_t width() const noexcept { return width_; }
        uint32_t height() const noexcept { return height_; }
        bool is_initialized() const noexcept { return encoder_ != nullptr; }

    private:
        struct Impl;
        std::unique_ptr<Impl> pimpl_;

        // PIMPL exposed internals (used by Impl)
        void* encoder_ = nullptr;
        ID3D11Device* d3d_device_ = nullptr;
        ID3D11DeviceContext* d3d_context_ = nullptr;
        uint32_t width_ = 0;
        uint32_t height_ = 0;
        uint32_t target_bitrate_ = 4'000'000;

        static constexpr size_t NUM_BUFFERS = 3;
        std::array<void*, NUM_BUFFERS> registered_textures_{};
        std::array<void*, NUM_BUFFERS> output_buffers_{};
        std::array<void*, NUM_BUFFERS> mapped_resources_{};
        uint32_t current_buf_idx_ = 0;

        EncodeCallback callback_;
        bool pending_idr_ = false;

        bool allocate_io_buffers();
        void process_encoded_frame(uint32_t buf_idx, uint64_t timestamp_us);
        void release_encoder_resources();
    };

} // namespace rapiddesk::codec
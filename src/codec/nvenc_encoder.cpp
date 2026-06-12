#include "nvenc_encoder.hpp"

#include <cstring>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>

#include <d3d11.h>
#include <nvEncodeAPI.h>

// NVENC GUIDs
static const GUID NV_ENC_CODEC_H264_GUID = { 0x60b8c8, 0xfd9c, 0x4b24, {0x80, 0x2b, 0x1f, 0x1, 0x6, 0x14, 0x14, 0x14} };
static const GUID NV_ENC_PRESET_P1_GUID = { 0x60b8c8, 0xfd9c, 0x4b24, {0x80, 0x2b, 0x1f, 0x1, 0x6, 0x14, 0x14, 0x14} }; // Fastest
static const GUID NV_ENC_PRESET_LOW_LATENCY_HQ_GUID = { 0x60b8c8, 0xfd9c, 0x4b24, {0x80, 0x2b, 0x1f, 0x1, 0x6, 0x14, 0x14, 0x14} };
static const GUID NV_ENC_PARAMS_RC_CBR_LOWDELAY_HQ = { 0x60b8c8, 0xfd9c, 0x4b24, {0x80, 0x2b, 0x1f, 0x1, 0x6, 0x14, 0x14, 0x14} };

// NVENC API function pointers
typedef NVENCSTATUS(NVENCAPI* NvEncOpenEncodeSessionEx_t)(NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS*, void**);
typedef NVENCSTATUS(NVENCAPI* NvEncInitializeEncoder_t)(void*, NV_ENC_INITIALIZE_PARAMS*);
typedef NVENCSTATUS(NVENCAPI* NvEncRegisterResource_t)(void*, NV_ENC_REGISTER_RESOURCE*);
typedef NVENCSTATUS(NVENCAPI* NvEncMapInputResource_t)(void*, NV_ENC_MAP_INPUT_RESOURCE*);
typedef NVENCSTATUS(NVENCAPI* NvEncUnmapInputResource_t)(void*, NV_ENC_INPUT_PTR);
typedef NVENCSTATUS(NVENCAPI* NvEncCreateBitstreamBuffer_t)(void*, NV_ENC_CREATE_BITSTREAM_BUFFER*);
typedef NVENCSTATUS(NVENCAPI* NvEncDestroyBitstreamBuffer_t)(void*, NV_ENC_OUTPUT_PTR);
typedef NVENCSTATUS(NVENCAPI* NvEncEncodePicture_t)(void*, NV_ENC_PIC_PARAMS*);
typedef NVENCSTATUS(NVENCAPI* NvEncLockBitstream_t)(void*, NV_ENC_LOCK_BITSTREAM*);
typedef NVENCSTATUS(NVENCAPI* NvEncUnlockBitstream_t)(void*, NV_ENC_OUTPUT_PTR);
typedef NVENCSTATUS(NVENCAPI* NvEncReconfigureEncoder_t)(void*, NV_ENC_RECONFIGURE_PARAMS*);
typedef NVENCSTATUS(NVENCAPI* NvEncGetEncodePresetConfig_t)(void*, GUID, GUID, NV_ENC_PRESET_CONFIG*);
typedef NVENCSTATUS(NVENCAPI* NvEncDestroyEncoder_t)(void*);

namespace rapiddesk::codec {

    struct NVEncEncoder::Impl {
        HMODULE nvenc_lib = nullptr;
        void* encoder = nullptr;

        // NVENC function pointers
        NvEncOpenEncodeSessionEx_t    fnOpenSession = nullptr;
        NvEncInitializeEncoder_t      fnInitEncoder = nullptr;
        NvEncRegisterResource_t       fnRegisterResource = nullptr;
        NvEncMapInputResource_t       fnMapInputResource = nullptr;
        NvEncUnmapInputResource_t     fnUnmapInputResource = nullptr;
        NvEncCreateBitstreamBuffer_t  fnCreateBitstreamBuffer = nullptr;
        NvEncDestroyBitstreamBuffer_t fnDestroyBitstreamBuffer = nullptr;
        NvEncEncodePicture_t          fnEncodePicture = nullptr;
        NvEncLockBitstream_t          fnLockBitstream = nullptr;
        NvEncUnlockBitstream_t        fnUnlockBitstream = nullptr;
        NvEncReconfigureEncoder_t     fnReconfigure = nullptr;
        NvEncGetEncodePresetConfig_t  fnGetPresetConfig = nullptr;
        NvEncDestroyEncoder_t         fnDestroyEncoder = nullptr;

        bool load_nvenc() {
            nvenc_lib = LoadLibraryA("nvEncodeAPI64.dll");
            if (!nvenc_lib) return false;

            fnOpenSession = (NvEncOpenEncodeSessionEx_t)GetProcAddress(nvenc_lib, "NvEncOpenEncodeSessionEx");
            fnInitEncoder = (NvEncInitializeEncoder_t)GetProcAddress(nvenc_lib, "NvEncInitializeEncoder");
            fnRegisterResource = (NvEncRegisterResource_t)GetProcAddress(nvenc_lib, "NvEncRegisterResource");
            fnMapInputResource = (NvEncMapInputResource_t)GetProcAddress(nvenc_lib, "NvEncMapInputResource");
            fnUnmapInputResource = (NvEncUnmapInputResource_t)GetProcAddress(nvenc_lib, "NvEncUnmapInputResource");
            fnCreateBitstreamBuffer = (NvEncCreateBitstreamBuffer_t)GetProcAddress(nvenc_lib, "NvEncCreateBitstreamBuffer");
            fnDestroyBitstreamBuffer = (NvEncDestroyBitstreamBuffer_t)GetProcAddress(nvenc_lib, "NvEncDestroyBitstreamBuffer");
            fnEncodePicture = (NvEncEncodePicture_t)GetProcAddress(nvenc_lib, "NvEncEncodePicture");
            fnLockBitstream = (NvEncLockBitstream_t)GetProcAddress(nvenc_lib, "NvEncLockBitstream");
            fnUnlockBitstream = (NvEncUnlockBitstream_t)GetProcAddress(nvenc_lib, "NvEncUnlockBitstream");
            fnReconfigure = (NvEncReconfigureEncoder_t)GetProcAddress(nvenc_lib, "NvEncReconfigureEncoder");
            fnGetPresetConfig = (NvEncGetEncodePresetConfig_t)GetProcAddress(nvenc_lib, "NvEncGetEncodePresetConfig");
            fnDestroyEncoder = (NvEncDestroyEncoder_t)GetProcAddress(nvenc_lib, "NvEncDestroyEncoder");

            return fnOpenSession && fnInitEncoder && fnEncodePicture && fnDestroyEncoder;
        }
    };

    NVEncEncoder::NVEncEncoder() : pimpl_(std::make_unique<Impl>()) {}
    NVEncEncoder::~NVEncEncoder() { release_encoder_resources(); }

    bool NVEncEncoder::initialize(ID3D11Device* d3d_device,
        uint32_t width,
        uint32_t height,
        uint32_t bitrate_bps) {
        if (!pimpl_->load_nvenc()) return false;

        d3d_device_ = d3d_device;
        d3d_device->GetImmediateContext(&d3d_context_);
        width_ = width;
        height_ = height;
        target_bitrate_ = bitrate_bps;

        // Open encode session with D3D11 device
        NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS session_params = {};
        session_params.version = NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS_VER;
        session_params.deviceType = NV_ENC_DEVICE_TYPE_DIRECTX;
        session_params.device = d3d_device;
        session_params.apiVersion = NVENCAPI_VERSION;

        NVENCSTATUS status = pimpl_->fnOpenSession(&session_params, &pimpl_->encoder);
        if (status != NV_ENC_SUCCESS) return false;
        encoder_ = pimpl_->encoder;

        // Get preset config and modify for low latency
        NV_ENC_PRESET_CONFIG preset_config = {};
        preset_config.version = NV_ENC_PRESET_CONFIG_VER;
        preset_config.presetCfg.version = NV_ENC_CONFIG_VER;

        status = pimpl_->fnGetPresetConfig(encoder_, NV_ENC_CODEC_H264_GUID,
            NV_ENC_PRESET_P1_GUID, &preset_config);
        if (status != NV_ENC_SUCCESS) return false;

        // Configure for ultra-low latency
        NV_ENC_CONFIG& enc_config = preset_config.presetCfg;
        enc_config.frameIntervalP = 1;           // No B-frames
        enc_config.gopLength = NVENC_INFINITE_GOPLENGTH;
        enc_config.rcParams.rateControlMode = NV_ENC_PARAMS_RC_CBR_LOWDELAY_HQ;
        enc_config.rcParams.averageBitRate = target_bitrate_;
        enc_config.rcParams.maxBitRate = static_cast<uint32_t>(target_bitrate_ * 1.5);
        enc_config.rcParams.vbvBufferSize = target_bitrate_ / 4;      // 250ms
        enc_config.rcParams.vbvInitialDelay = target_bitrate_ / 4;

        // H.264 specific low-latency tuning
        auto& h264 = enc_config.encodeCodecConfig.h264Config;
        h264.idrPeriod = NVENC_INFINITE_GOPLENGTH;
        h264.sliceMode = 3;
        h264.sliceModeData = 1;                 // 1 slice per frame
        h264.repeatSPSPPS = 1;
        h264.outputAUD = 0;
        h264.level = NV_ENC_LEVEL_H264_51;

        NV_ENC_INITIALIZE_PARAMS init_params = {};
        init_params.version = NV_ENC_INITIALIZE_PARAMS_VER;
        init_params.encodeGUID = NV_ENC_CODEC_H264_GUID;
        init_params.presetGUID = NV_ENC_PRESET_P1_GUID;
        init_params.encodeWidth = width_;
        init_params.encodeHeight = height_;
        init_params.frameRateNum = 60;
        init_params.frameRateDen = 1;
        init_params.enablePTD = 1;
        init_params.encodeConfig = &enc_config;

        status = pimpl_->fnInitEncoder(encoder_, &init_params);
        if (status != NV_ENC_SUCCESS) {
            release_encoder_resources();
            return false;
        }

        return allocate_io_buffers();
    }

    bool NVEncEncoder::allocate_io_buffers() {
        for (size_t i = 0; i < NUM_BUFFERS; ++i) {
            // Create staging textures for input
            D3D11_TEXTURE2D_DESC desc = {};
            desc.Width = width_;
            desc.Height = height_;
            desc.MipLevels = 1;
            desc.ArraySize = 1;
            desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
            desc.SampleDesc.Count = 1;
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.BindFlags = D3D11_BIND_RENDER_TARGET;

            ID3D11Texture2D* staging_tex = nullptr;
            HRESULT hr = d3d_device_->CreateTexture2D(&desc, nullptr, &staging_tex);
            if (FAILED(hr)) return false;

            // Register with NVENC
            NV_ENC_REGISTER_RESOURCE reg = {};
            reg.version = NV_ENC_REGISTER_RESOURCE_VER;
            reg.resourceType = NV_ENC_INPUT_RESOURCE_TYPE_DIRECTX;
            reg.resourceToRegister = staging_tex;
            reg.width = width_;
            reg.height = height_;
            reg.bufferFormat = NV_ENC_BUFFER_FORMAT_ARGB;

            NVENCSTATUS status = pimpl_->fnRegisterResource(encoder_, &reg);
            if (status != NV_ENC_SUCCESS) {
                staging_tex->Release();
                return false;
            }
            registered_textures_[i] = reg.registeredResource;

            // Create bitstream buffer for output
            NV_ENC_CREATE_BITSTREAM_BUFFER bitstream = {};
            bitstream.version = NV_ENC_CREATE_BITSTREAM_BUFFER_VER;
            status = pimpl_->fnCreateBitstreamBuffer(encoder_, &bitstream);
            if (status != NV_ENC_SUCCESS) return false;
            output_buffers_[i] = bitstream.bitstreamBuffer;
        }
        return true;
    }

    void NVEncEncoder::encode_texture(ID3D11Texture2D* texture,
        bool force_keyframe,
        uint64_t timestamp_us) {
        if (!encoder_) return;

        uint32_t idx = current_buf_idx_;
        current_buf_idx_ = (current_buf_idx_ + 1) % NUM_BUFFERS;

        // Map input resource
        NV_ENC_MAP_INPUT_RESOURCE map_params = {};
        map_params.version = NV_ENC_MAP_INPUT_RESOURCE_VER;
        map_params.registeredResource = registered_textures_[idx];

        pimpl_->fnMapInputResource(encoder_, &map_params);
        mapped_resources_[idx] = map_params.mappedResource;

        // Encode picture params
        NV_ENC_PIC_PARAMS pic_params = {};
        pic_params.version = NV_ENC_PIC_PARAMS_VER;
        pic_params.inputBuffer = map_params.mappedResource;
        pic_params.bufferFmt = NV_ENC_BUFFER_FORMAT_ARGB;
        pic_params.inputWidth = width_;
        pic_params.inputHeight = height_;
        pic_params.outputBitstream = output_buffers_[idx];
        pic_params.pictureStruct = NV_ENC_PIC_STRUCT_FRAME;
        pic_params.inputTimeStamp = timestamp_us;

        uint32_t flags = 0;
        if (force_keyframe || pending_idr_) {
            flags |= NV_ENC_PIC_FLAG_FORCEIDR | NV_ENC_PIC_FLAG_OUTPUT_SPSPPS;
            pending_idr_ = false;
        }
        pic_params.encodePicFlags = flags;

        NVENCSTATUS status = pimpl_->fnEncodePicture(encoder_, &pic_params);

        // Unmap immediately after encode (async processing on GPU)
        pimpl_->fnUnmapInputResource(encoder_, map_params.mappedResource);
        mapped_resources_[idx] = nullptr;

        if (status == NV_ENC_SUCCESS) {
            process_encoded_frame(idx, timestamp_us);
        }
    }

    void NVEncEncoder::process_encoded_frame(uint32_t buf_idx, uint64_t timestamp_us) {
        NV_ENC_LOCK_BITSTREAM lock_params = {};
        lock_params.version = NV_ENC_LOCK_BITSTREAM_VER;
        lock_params.outputBitstream = output_buffers_[buf_idx];
        lock_params.doNotWait = 0;

        NVENCSTATUS status = pimpl_->fnLockBitstream(encoder_, &lock_params);
        if (status != NV_ENC_SUCCESS) return;

        if (callback_) {
            EncodedFrame frame;
            frame.data = std::span<const uint8_t>(
                static_cast<const uint8_t*>(lock_params.bitstreamBufferPtr),
                lock_params.bitstreamSizeInBytes);
            frame.timestamp_us = timestamp_us;
            frame.is_keyframe = (lock_params.pictureType == NV_ENC_PIC_TYPE_IDR);
            frame.width = width_;
            frame.height = height_;
            callback_(frame);
        }

        pimpl_->fnUnlockBitstream(encoder_, output_buffers_[buf_idx]);
    }

    void NVEncEncoder::update_bitrate(uint32_t bitrate_bps) {
        if (!encoder_) return;
        target_bitrate_ = bitrate_bps;

        NV_ENC_RECONFIGURE_PARAMS reconfig = {};
        reconfig.version = NV_ENC_RECONFIGURE_PARAMS_VER;
        reconfig.reInitEncodeParams.version = NV_ENC_INITIALIZE_PARAMS_VER;
        reconfig.reInitEncodeParams.encodeConfig = nullptr; // Keep existing config
        reconfig.reInitEncodeParams.frameRateNum = 60;
        reconfig.reInitEncodeParams.frameRateDen = 1;

        // NVENC reconfigure for bitrate update
        // Note: Full implementation requires copying existing config and modifying bitrate
        // This is a simplified version
        pimpl_->fnReconfigure(encoder_, &reconfig);
    }

    void NVEncEncoder::force_idr() {
        pending_idr_ = true;
    }

    void NVEncEncoder::set_callback(EncodeCallback cb) {
        callback_ = std::move(cb);
    }

    void NVEncEncoder::release_encoder_resources() {
        if (!encoder_) return;

        for (size_t i = 0; i < NUM_BUFFERS; ++i) {
            if (output_buffers_[i]) {
                pimpl_->fnDestroyBitstreamBuffer(encoder_, output_buffers_[i]);
                output_buffers_[i] = nullptr;
            }
            if (registered_textures_[i]) {
                // NVENC unregister resource (not exposed in simplified API)
                registered_textures_[i] = nullptr;
            }
        }

        pimpl_->fnDestroyEncoder(encoder_);
        encoder_ = nullptr;
        pimpl_->encoder = nullptr;

        if (pimpl_->nvenc_lib) {
            FreeLibrary(pimpl_->nvenc_lib);
            pimpl_->nvenc_lib = nullptr;
        }
    }

} // namespace rapiddesk::codec
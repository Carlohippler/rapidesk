#include "codec/nvenc_encoder.hpp"

#ifdef RAPIDDESK_NVENC_ENABLED
#include <nvEncodeAPI.h>
// ... implementação real do NVENC
#else

namespace rapiddesk::codec {

    class NVEncEncoder::Impl {};

    NVEncEncoder::NVEncEncoder() = default;
    NVEncEncoder::~NVEncEncoder() = default;

    bool NVEncEncoder::initialize(int width, int height, ID3D11Device* device) {
        (void)width; (void)height; (void)device;
        return false;  // NVENC não disponível
    }

    void NVEncEncoder::set_callback(EncodeCallback cb) {
        callback_ = std::move(cb);
    }

    bool NVEncEncoder::encode_frame(ID3D11Texture2D* texture) {
        (void)texture;
        return false;
    }

    void NVEncEncoder::shutdown() {}

} // namespace rapiddesk::codec

#endif
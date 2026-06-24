#pragma once
#include "codec/frame_types.hpp"
#include <functional>

struct ID3D11Device;
struct ID3D11Texture2D;

namespace rapiddesk::codec {

    class NVEncEncoder {
    public:
        NVEncEncoder();
        ~NVEncEncoder();

        bool initialize(int width, int height, ID3D11Device* device = nullptr);
        void set_callback(EncodeCallback cb);
        bool encode_frame(ID3D11Texture2D* texture);
        void shutdown();

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;
        EncodeCallback callback_;
    };

} // namespace rapiddesk::codec
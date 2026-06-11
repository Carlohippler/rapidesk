// src/codec/nvenc_encoder.hpp
#pragma once
#include <vector>
#include <cstdint>

class ID3D11Texture2D;

class NVENCEncoderMock {
public:
    void encode_texture(ID3D11Texture2D* tex, std::vector<uint8_t>& output_bitstream);
    void update_bitrate(uint32_t bps);
};
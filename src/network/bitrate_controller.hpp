// src/network/bitrate_controller.hpp
#pragma once
#include "../codec/nvenc_encoder.hpp" // Para interagir com o encoder

class BitrateController {
public:
    void evaluate_network(float loss_rate, uint32_t rtt_ms, NVENCEncoderMock& encoder);
private:
    uint32_t current_bitrate_ = 2'000'000;
};
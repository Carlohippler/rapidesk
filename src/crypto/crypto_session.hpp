// src/crypto/crypto_session.hpp
#pragma once
#include <vector>
#include <array>
#include <atomic>
#include <span>
#include <openssl/evp.h>

class CryptoSession {
public:
    CryptoSession();
    ~CryptoSession();
    bool initialize_e2e();
    bool encrypt_packet(std::span<const uint8_t> plaintext, std::vector<uint8_t>& ciphertext);
private:
    EVP_PKEY* local_keypair_ = nullptr;
    std::array<uint8_t, 32> aes_key_{ {0x01} };
    std::atomic<uint64_t> send_counter_;
};
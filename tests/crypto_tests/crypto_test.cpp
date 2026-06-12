#include <gtest/gtest.h>
#include "crypto/crypto_session.hpp"
#include "crypto/auth.hpp"
#include <vector>
#include <string>

using namespace rapiddesk::crypto;

class CryptoSessionTest : public ::testing::Test {
protected:
    void SetUp() override {
        alice_ = std::make_unique<CryptoSession>();
        bob_ = std::make_unique<CryptoSession>();
    }

    std::unique_ptr<CryptoSession> alice_;
    std::unique_ptr<CryptoSession> bob_;
};

TEST_F(CryptoSessionTest, KeyExchangeSuccess) {
    ASSERT_TRUE(alice_->generate_ephemeral_keypair());
    ASSERT_TRUE(bob_->generate_ephemeral_keypair());

    auto alice_pub = alice_->public_key();
    auto bob_pub = bob_->public_key();

    ASSERT_TRUE(alice_->derive_session_key(bob_pub));
    ASSERT_TRUE(bob_->derive_session_key(alice_pub));

    // Keys should match after ECDH
    EXPECT_TRUE(alice_->session_key_valid());
    EXPECT_TRUE(bob_->session_key_valid());
}

TEST_F(CryptoSessionTest, EncryptDecryptRoundtrip) {
    alice_->generate_ephemeral_keypair();
    bob_->generate_ephemeral_keypair();

    alice_->derive_session_key(bob_->public_key());
    bob_->derive_session_key(alice_->public_key());

    std::string plaintext = "Hello, RapidDesk! Secure messaging test.";
    std::vector<uint8_t> data(plaintext.begin(), plaintext.end());

    auto ciphertext = alice_->encrypt(data);
    EXPECT_GT(ciphertext.size(), data.size()); // Should have tag + nonce

    auto decrypted = bob_->decrypt(ciphertext);
    ASSERT_TRUE(decrypted.has_value());
    EXPECT_EQ(data, *decrypted);
}

TEST_F(CryptoSessionTest, AntiReplayProtection) {
    alice_->generate_ephemeral_keypair();
    bob_->generate_ephemeral_keypair();
    alice_->derive_session_key(bob_->public_key());
    bob_->derive_session_key(alice_->public_key());

    std::vector<uint8_t> data = { 0x01, 0x02, 0x03, 0x04 };
    auto ciphertext = alice_->encrypt(data);

    // First decrypt should succeed
    auto result1 = bob_->decrypt(ciphertext);
    ASSERT_TRUE(result1.has_value());

    // Replay should fail (same nonce)
    auto result2 = bob_->decrypt(ciphertext);
    EXPECT_FALSE(result2.has_value());
}

TEST_F(CryptoSessionTest, TamperDetection) {
    alice_->generate_ephemeral_keypair();
    bob_->generate_ephemeral_keypair();
    alice_->derive_session_key(bob_->public_key());
    bob_->derive_session_key(alice_->public_key());

    auto ciphertext = alice_->encrypt({ 0xAB, 0xCD, 0xEF });

    // Tamper with ciphertext
    ciphertext[ciphertext.size() / 2] ^= 0xFF;

    auto result = bob_->decrypt(ciphertext);
    EXPECT_FALSE(result.has_value());
}

// --- Auth Tests ---

TEST(AuthTest, Argon2Hashing) {
    std::string password = "SecurePassword123!";
    std::string salt = Auth::generate_salt();

    auto hash1 = Auth::hash_password(password, salt);
    auto hash2 = Auth::hash_password(password, salt);

    EXPECT_EQ(hash1, hash2); // Deterministic
    EXPECT_NE(hash1, Auth::hash_password("WrongPassword", salt));
}

TEST(AuthTest, ConstantTimeComparison) {
    std::string a = "secret_key_12345";
    std::string b = "secret_key_12345";
    std::string c = "secret_key_99999";

    EXPECT_TRUE(Auth::secure_compare(a, b));
    EXPECT_FALSE(Auth::secure_compare(a, c));
}

TEST(AuthTest, SessionTokenGeneration) {
    auto token1 = Auth::generate_session_token();
    auto token2 = Auth::generate_session_token();

    EXPECT_EQ(token1.size(), 32); // 256 bits
    EXPECT_NE(token1, token2);    // Random
}
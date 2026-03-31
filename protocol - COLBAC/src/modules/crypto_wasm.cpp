//
// Created by Catarina on 23/01/2026.
//
#include <sodium.h>
#include <emscripten/bind.h>
#include <string>
#include <vector>

using namespace emscripten;

/* ---------- Helpers ---------- */

static std::vector<unsigned char> from_hex(const std::string& hex) {
    std::vector<unsigned char> out(hex.size() / 2);
    size_t bin_len;

    if (sodium_hex2bin(
            out.data(), out.size(),
            hex.c_str(), hex.size(),
            nullptr, &bin_len, nullptr) != 0 ||
        bin_len != out.size()) {
        return {};
    }
    return out;
}

static std::string to_hex(const unsigned char* data, size_t len) {
    std::string hex(len * 2, '\0');
    sodium_bin2hex(hex.data(), hex.size() + 1, data, len);
    return hex;
}

/* ---------- API ---------- */

// Ristretto DH
std::string derive_shared_secret_hex(
        const std::string& privHex,
        const std::string& pubHex
) {
    static bool ready = false;
    if (!ready) {
        if (sodium_init() < 0)
            throw std::runtime_error("libsodium init failed");
        ready = true;
    }

    auto sk = from_hex(privHex);
    auto pk = from_hex(pubHex);

    if (sk.size() != crypto_core_ristretto255_SCALARBYTES ||
        pk.size() != crypto_core_ristretto255_BYTES)
        throw std::runtime_error("Invalid key size");

    if (crypto_core_ristretto255_is_valid_point(pk.data()) != 1)
        throw std::runtime_error("Invalid Ristretto point");

    unsigned char out[crypto_core_ristretto255_BYTES];

    if (crypto_scalarmult_ristretto255(out, sk.data(), pk.data()) != 0)
        throw std::runtime_error("Scalar multiplication failed");

    return to_hex(out, sizeof(out));
}

// HMAC using HMAC-SHA256 instead of crypto_auth
std::string create_hmac_hex(
        const std::string& sharedSecretHex,
        const std::string& message
) {
    auto key = from_hex(sharedSecretHex);

    // Use HMAC-SHA256 instead of crypto_auth
    unsigned char mac[crypto_auth_hmacsha256_BYTES];

    crypto_auth_hmacsha256(
        mac,
        reinterpret_cast<const unsigned char*>(message.data()),
        message.size(),
        key.data()
    );

    return to_hex(mac, crypto_auth_hmacsha256_BYTES);
}

bool verify_hmac_hex(
        const std::string& sharedSecretHex,
        const std::string& message,
        const std::string& macHex
) {
    auto key = from_hex(sharedSecretHex);
    auto mac = from_hex(macHex);

    if (mac.size() != crypto_auth_hmacsha256_BYTES)
        return false;

    return crypto_auth_hmacsha256_verify(
        mac.data(),
        reinterpret_cast<const unsigned char*>(message.data()),
        message.size(),
        key.data()
    ) == 0;
}

/* ---------- Bindings ---------- */

EMSCRIPTEN_BINDINGS(crypto_module) {
    function("derive_shared_secret_hex", &derive_shared_secret_hex);
    function("create_hmac_hex", &create_hmac_hex);
    function("verify_hmac_hex", &verify_hmac_hex);
}

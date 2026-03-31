//
// Created by Catarina on 03/07/2025.
//
#include <sodium.h>
#include <emscripten/emscripten.h>
#include "../parties.hpp"
#include "../verifiable_exponentiation.hpp"
#include <string>
#include <sstream>
#include <iomanip>
#include "../utilities.hpp"

// Exported function (to run on JavaScript)
extern "C" {
    EMSCRIPTEN_KEEPALIVE
    const char* group_init_exp(size_t ell, const char* tag_list);
}


const char* group_init_exp(size_t ell, const char* tag_list) {
    static std::string result;

    // Initialize sodium for the random generation below
    if (sodium_init() < 0) {
        return nullptr;
    }

    // Convert `tag_list` (space-separated hex strings) into vector of byte arrays
    std::istringstream stream(tag_list);
    std::string hex_token;
    std::vector<unsigned char*> user_tokens;

    while (stream >> hex_token) {
        std::vector<uint8_t> bytes = from_hex(hex_token);
        if (bytes.size() != crypto_core_ristretto255_BYTES) {
            result = "Error: Invalid token size in input";
            return result.c_str();
        }

        unsigned char* token_ptr = new unsigned char[crypto_core_ristretto255_BYTES];
        memcpy(token_ptr, bytes.data(), crypto_core_ristretto255_BYTES);
        user_tokens.push_back(token_ptr);
    }

    if (user_tokens.size() != ell) {
        result = "Error: Token count mismatch with 'ell'";
        return result.c_str();
    }


    // G.InitExp
    std::cout << "G.InitExp..." << std::endl;

    // RO salt
    unsigned char salt[crypto_hash_sha256_BYTES];
    randombytes_buf(salt, crypto_hash_sha256_BYTES);

    // generate alpha obfuscator scalar
    unsigned char alpha[crypto_core_ristretto255_SCALARBYTES];
    assert(2 * crypto_core_ristretto255_SCALARBYTES >= crypto_hash_sha256_BYTES);
    unsigned char alpha_hash[2 * crypto_core_ristretto255_SCALARBYTES]; // read note on crypto_core_ristretto255_scalar_reduce
    memset(alpha_hash, 0, 2 * crypto_core_ristretto255_SCALARBYTES);
    crypto_hash_sha256_state state;
    crypto_hash_sha256_init(&state);
    crypto_hash_sha256_update(&state, salt, crypto_hash_sha256_BYTES);
    /*for (auto const& x : user_tokens)
    {
        std::string key = x.first;
        unsigned char *val = x.second;
        crypto_hash_sha256_update(&state, val, crypto_core_ristretto255_BYTES);
    }*/

    for (const auto& val : user_tokens) {
        crypto_hash_sha256_update(&state, val, crypto_core_ristretto255_BYTES);
    }
    crypto_hash_sha256_final(&state, alpha_hash);
    crypto_core_ristretto255_scalar_reduce(alpha, alpha_hash);

    // generate obfuscated tag list T1, ..., Tn
    unsigned char *T[ell];
    for (auto i = 0; i < ell; i++)
    {
        T[i] = (unsigned char *)malloc(crypto_core_ristretto255_BYTES);
    }
    /*size_t ctr = 0;
    for (auto const& x : user_tokens)
    {
        std::string key = x.first;
        unsigned char *val = x.second;
        if (crypto_scalarmult_ristretto255(T[ctr], alpha, val) != 0)
        {
            printf("failed to compute T[%zu]\n", ctr);
            abort();
        }
        ctr++;
    }*/

    for (size_t i = 0; i < ell; ++i) {
        T[i] = (unsigned char*)malloc(crypto_core_ristretto255_BYTES);
        if (crypto_scalarmult_ristretto255(T[i], alpha, user_tokens.at(i)) != 0) {
            printf("failed to compute T[%zu]\n", i);
            abort();
        }
    }


    // Optionally print
    /*for (size_t i = 0; i < ell; ++i) {
        std::cout << "T[" << i << "] = ";
        for (size_t j = 0; j < crypto_core_ristretto255_BYTES; ++j)
            printf("%02x", T[i][j]);
        std::cout << std::endl;
    }*/

    //std::cout << to_hex(alpha, crypto_core_ristretto255_SCALARBYTES) << std::endl;

    result = "{";
    result += "\"alpha\":\"" + to_hex(alpha, crypto_core_ristretto255_SCALARBYTES) + "\", ";
    result += "\"T\":\"";
    for (size_t i = 0; i < ell; ++i) {
        result += to_hex(T[i], crypto_core_ristretto255_BYTES);
        if (i < ell - 1) result += " ";
        free(T[i]);
    }
    result += "\"}";

    return result.c_str(); // Emscript copies the result to JavaScript

}





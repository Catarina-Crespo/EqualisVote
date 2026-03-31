//
// Created by Catarina on 02/06/2025.
//
#include <sodium.h>
#include <emscripten/emscripten.h>
#include "../parties.hpp"
#include "../batched_dleq.hpp"
#include "../verifiable_exponentiation.hpp"
#include <string>
#include <sstream>
#include <iomanip>
#include "../utilities.hpp"

// Exported function (to run on JavaScript)
extern "C" {
    EMSCRIPTEN_KEEPALIVE
    const char* generate_user_keys();
}

const char* generate_user_keys() {
    static std::string result;

    // Initialize sodium for the random generation below
    if (sodium_init() < 0) {
        return nullptr;
    }

    User user;
    VEP<BatchedDLEQProof> ve(nullptr);

    // Random generation
    crypto_core_ristretto255_scalar_random(user.v_i);

    ve.gen(user.u_i, user.upk_u_i, user.upk_proof);

    // Build JSON
    result = "{";
    result += "\"v_i\":\"" + to_hex(user.v_i, crypto_core_ristretto255_SCALARBYTES) + "\",";
    result += "\"u_i\":\"" + to_hex(user.u_i, crypto_core_ristretto255_SCALARBYTES) + "\",";
    result += "\"upk_u_i\":\"" + to_hex(user.upk_u_i, crypto_core_ristretto255_BYTES) + "\",";
    result += "\"upk_proof\":\"" + to_hex(user.upk_proof, DLOG2GenProof::proof_BYTES) + "\"";
    result += "}";

    return result.c_str(); // Emscript copies the result to JavaScript
}

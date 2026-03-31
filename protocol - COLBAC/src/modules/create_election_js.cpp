//
// Created by Catarina on 16/08/2025.
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
    const char* create_election();
}

const char* create_election() {
    static std::string result;

    // Initialize sodium for the random generation below
    if (sodium_init() < 0) {
        return nullptr;
    }

    VEP<BatchedDLEQProof> ve(NULL);
    unsigned char e_E[crypto_core_ristretto255_SCALARBYTES];
    unsigned char epk_E[crypto_core_ristretto255_BYTES];
    unsigned char epk_proof[DLOG2GenProof::proof_BYTES];

    // Generate group-specific key
    ve.gen(e_E, epk_E, epk_proof);

    // Build JSON
    result = "{";
    result += "\"e_E\":\"" + to_hex(e_E, crypto_core_ristretto255_SCALARBYTES) + "\",";
    result += "\"epk_E\":\"" + to_hex(epk_E, crypto_core_ristretto255_BYTES) + "\",";
    result += "\"epk_proof\":\"" + to_hex(epk_proof, DLOG2GenProof::proof_BYTES) + "\"";
    result += "}";

    return result.c_str(); // Emscript copies the result to JavaScript
}

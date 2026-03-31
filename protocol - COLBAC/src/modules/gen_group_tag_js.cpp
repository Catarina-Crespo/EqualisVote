//
// Created by Catarina on 02/06/2025.
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
    const char* generate_group_tag(const char* upk_u_i_hex, const char* v_i_hex, const char* spk_G_hex);
}

const char* generate_group_tag(const char* upk_u_i_hex, const char* v_i_hex, const char* spk_G_hex) {
    static std::string result;

    // Initialize sodium for the random generation below
    if (sodium_init() < 0) {
        return nullptr;
    }

    DLOGProof dlog_pf;

    std::vector<uint8_t> upk_u_i_vec = from_hex(upk_u_i_hex);
    unsigned char* upk_u_i = upk_u_i_vec.data();

    std::vector<uint8_t> v_i_vec = from_hex(v_i_hex);
    unsigned char* v_i = v_i_vec.data();

    std::vector<uint8_t> spk_G_vec = from_hex(spk_G_hex);
    unsigned char* spk_G = spk_G_vec.data();

    // Compute -v_i
    unsigned char neg_v_i[crypto_core_ristretto255_SCALARBYTES];
    crypto_core_ristretto255_scalar_negate(neg_v_i, v_i);

    // Compute z_i_G = (spk_G)^(-v_i)
    unsigned char *z_i_G = (unsigned char *)malloc(crypto_core_ristretto255_BYTES); // freed during Group destruction
    if (crypto_scalarmult_ristretto255(z_i_G, neg_v_i, spk_G) != 0)
    {
        printf("failed computing z_i_G\n");
        abort();
    }
    // compute a proof for z_i_G
    unsigned char user_token_statement[DLOGProof::statement_BYTES];
    unsigned char user_token_proof[DLOGProof::proof_BYTES];
    memcpy(user_token_statement, spk_G, crypto_core_ristretto255_BYTES);
    memcpy(user_token_statement + crypto_core_ristretto255_BYTES, z_i_G, crypto_core_ristretto255_BYTES);
    dlog_pf.prove(neg_v_i, user_token_statement, user_token_proof);

    // Build JSON
    result = "{";
    result += "\"upk_u_i\":\"" + hex_to_string(upk_u_i, crypto_core_ristretto255_BYTES) + "\",";
    result += "\"z_i_G\":\"" + hex_to_string(z_i_G, crypto_core_ristretto255_BYTES) + "\",";
    result += "\"user_token_statement\":\"" + hex_to_string(user_token_statement, DLOGProof::statement_BYTES) + "\",";
    result += "\"user_token_proof\":\"" + hex_to_string(user_token_proof, DLOGProof::proof_BYTES) + "\"";
    result += "}";

    return result.c_str(); // Emscript copies the result to JavaScript

}





//
// Created by Catarina on 16/06/2025.
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
    const char* encrypt_vote(const char* v_i_hex, const char* upk_u_i_hex, int vote);
}

// v_i (voter), upk_u_i (votee)
const char* encrypt_vote(const char* v_i_hex, const char* upk_u_i_hex, int vote) {
    static std::string result;

    // Initialize sodium for the random generation below
    if (sodium_init() < 0) {
        return nullptr;
    }

    DLOGProof dlog_pf;

    // Necessary conversion back from hex
    std::vector<uint8_t> v_i_vec = from_hex(v_i_hex);
    unsigned char* v_i = v_i_vec.data();

    std::vector<uint8_t> upk_u_i_vec = from_hex(upk_u_i_hex);
    unsigned char* upk_u_i = upk_u_i_vec.data();

    printf("Received v_i_hex: %s", v_i_hex);
    printf("Received upk_u_i_hex: %s", upk_u_i_hex);
    printf("Received vote: %d", vote);

    // Encode vote = x_{i,j} from domain D
    unsigned char x_i_j[crypto_core_ristretto255_SCALARBYTES]; // x_{i,j}
    memset(x_i_j, 0x00, crypto_core_ristretto255_SCALARBYTES);
    x_i_j[0] = (unsigned char)vote;

    // Calculate y_{i,j} = (upk_{u_j})^{v_i} * g^{x_{i,j}}
    unsigned char upk_v[crypto_core_ristretto255_BYTES];  // Holds upk_{u_j}^{v_i}

    // Compute upk_{u_j}^{v_i}
    if (crypto_scalarmult_ristretto255(upk_v, v_i, upk_u_i) != 0) {
        printf("failed computing upk^v\n");
        abort();
    }

    // Compute g^{x_{i,j}}
    unsigned char g_x_i_j[crypto_core_ristretto255_BYTES];
    crypto_scalarmult_ristretto255_base(g_x_i_j, x_i_j);

    // Now multiply temp (upk_{u_j}^{v_i}) with g^{x_{i,j}}
    unsigned char *ballot = (unsigned char *)malloc(crypto_core_ristretto255_BYTES); // freed during server destruction
    crypto_core_ristretto255_add(ballot, upk_v, g_x_i_j);

    // Build JSON
    result = "{";
    result += "\"vote\":\"" + std::to_string(vote) + "\",";
    result += "\"ballot\":\"" + hex_to_string(ballot, crypto_core_ristretto255_BYTES) + "\"";
    result += "}";

    return result.c_str(); // Emscript copies the result to JavaScript

}





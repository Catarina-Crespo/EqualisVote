//
// Created by Catarina on 25/06/2025.
//
#include <sodium.h>
#include <emscripten/emscripten.h>
#include "../parties.hpp"
#include "../batched_dleq.hpp"
#include "../verifiable_exponentiation.hpp"
#include "../random_permutation.hpp"
#include <string>
#include <sstream>
#include <iomanip>

#include <iostream>
#include <vector>
#include <algorithm>
#include <cassert>
#include <cstring>
#include "../parties.hpp"
#include "../random_permutation.hpp"
#include "../verifiable_exponentiation.hpp"
#include "../fiat_shamir.hpp"
#include "../repeated_sigma_protocol.hpp"
#include "../shuffle_compatible_dleq.hpp"
#include "../shuffled_sigma_protocol.hpp"
#include "../utilities.hpp"
#include "../base_point.hpp"

// Exported function (to run on JavaScript)
extern "C" {
    EMSCRIPTEN_KEEPALIVE
    const char* shuffle_user(
        const char* u_i_hex,
        const char* upk_u_i_hex,
        const char* Tprime_hex_list // space-separated hex tokens
    );
}

// ell is the group_size
const char* shuffle_user(const char* u_i_hex, const char* upk_u_i_hex, const char* Tprime_hex_list) {
    static std::string result;

    // Initialize sodium for the random generation below
    if (sodium_init() < 0) {
        return nullptr;
    }

    /*std::cout << "Printing u_i: ";
    std::cout <<  u_i_hex << std::endl;
    std::cout << "Printing upk: ";
    std::cout <<  upk_u_i_hex << std::endl;*/

    // Convert inputs
    std::vector<uint8_t> u_i_vec = from_hex(u_i_hex);
    unsigned char* u_i = u_i_vec.data();
    std::vector<uint8_t> upk_u_i_vec = from_hex(upk_u_i_hex);
    unsigned char* upk_u_i = upk_u_i_vec.data();

    // Parse the token list
    std::vector<std::string> T_hex_tokens = extract_tokens(Tprime_hex_list, ' ');
    size_t ell = T_hex_tokens.size();

    /*std::cout << "Tprime_hex_list: " << Tprime_hex_list << std::endl;
    std::cout << "Tokens extracted: " << T_hex_tokens.size() << std::endl;
    for (const auto& token : T_hex_tokens) {
        std::cout << token << std::endl;
    }*/


    // Convert to Tprime
    // unsigned char* Tprime[ell];
    std::vector<unsigned char*> Tprime(ell);
    for (size_t i = 0; i < ell; ++i) {

        if (i >= T_hex_tokens.size()) {
            std::cerr << "Error: index " << i << " out of range for T_hex_tokens." << std::endl;
            result = "Error: Token index out of range";
            return result.c_str();
        }

        std::vector<uint8_t> tok = from_hex(T_hex_tokens.at(i));

        if (tok.size() != crypto_core_ristretto255_BYTES) {
            std::cerr << "Invalid token length at index " << i << ": " << tok.size() << " bytes" << std::endl;
            result = "Error: Invalid token length";
            return result.c_str();
        }

        if (crypto_core_ristretto255_is_valid_point(tok.data()) != 1) {
            std::cerr << "Tprime[" << i << "] is not a valid Ristretto point." << std::endl;
            result = "Error: Invalid point in Tprime";
            return result.c_str();
        }

        Tprime[i] = (unsigned char*)malloc(crypto_core_ristretto255_BYTES);
        memcpy(Tprime[i], tok.data(), crypto_core_ristretto255_BYTES);
    }

    std::cout << "U.InitCount..." << std::endl;
    //size_t sigma_i_star_G[ell];
    std::vector<size_t> sigma_i_star_G(ell);
    random_permutation(ell, sigma_i_star_G.data());

    std::cout << "Printing sigma_i_star_G" << std::endl;
    for (auto i = 0; i < ell; i++) {
        std::cout << sigma_i_star_G[i];
        if (i < ell - 1) std::cout << " ";
    }
    std::cout << std::endl;

    ShuffledSigmaProtocol<SCDLEQSigmaProtocol> pfsys(ell);
    RepeatedSigmaProtocol<ShuffledSigmaProtocol<SCDLEQSigmaProtocol>> repfsys(128, pfsys);
    FiatShamir<RepeatedSigmaProtocol<ShuffledSigmaProtocol<SCDLEQSigmaProtocol>>> fsproof(repfsys);
    VEP<FiatShamir<RepeatedSigmaProtocol<ShuffledSigmaProtocol<SCDLEQSigmaProtocol>>>> vep(&fsproof);


    // U.ShuffleExp
    std::cout << "U.ShuffleExp..." << std::endl;
    //unsigned char *Tpp[ell];
    std::vector<unsigned char*> Tpp(ell);

    for (auto i = 0; i < ell; i++) {
        Tpp[i] = (unsigned char *) malloc(crypto_core_ristretto255_BYTES);
    }

    //unsigned char U_shuffle_exp_proof[vep.exponentiation_proof_BYTES];
    unsigned char* U_shuffle_exp_proof = new unsigned char[vep.exponentiation_proof_BYTES];
    std::memset(U_shuffle_exp_proof, 0, vep.exponentiation_proof_BYTES);

    if (vep.eval(
        u_i, upk_u_i,
        ell, Tprime.data(),
        sigma_i_star_G.data(),
        Tpp.data(),
        U_shuffle_exp_proof
    ) != 0) {
        assert(false);
    }

    result = "{";
    result += "\"Tpp\":\"";

    std::cout << "Success! Printing Tpp..." << std::endl;
    for (auto i = 0; i < ell; i++) {
        //std::cout << to_hex(Tpp[i], crypto_core_ristretto255_BYTES) << std::endl;
        result += to_hex(Tpp[i], crypto_core_ristretto255_BYTES);
        if (i < ell - 1) result += " ";
    }
    result += "\"}";

    // check proof
    assert(vep.check(upk_u_i, ell, Tprime.data(), Tpp.data(), U_shuffle_exp_proof));
    delete[] U_shuffle_exp_proof;

    for (auto i = 0; i < ell; ++i) {
        free(Tprime[i]);
        free(Tpp[i]);
    }

    return result.c_str(); // Emscript copies the result to JavaScript
}

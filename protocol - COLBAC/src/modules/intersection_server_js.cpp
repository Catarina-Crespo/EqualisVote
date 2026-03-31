//
// Created by Catarina on 06/07/2025.
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
    const char* intersection(size_t ell,
                             const char* alpha_str,
                             const char* W_s_str,
                             const char* overline_spk_i_start_G_str,
                             const char* Tpp_str,
                             int min_vote,
                             int max_vote,
                             size_t n_ballots);
}


// ell is the group_size
const char* intersection(size_t ell,
                             const char* alpha_str,
                             const char* W_s_str,
                             const char* overline_spk_i_start_G_str,
                             const char* Tpp_str,
                             int min_vote,
                             int max_vote,
                             size_t n_ballots) {

    static std::string result;

    // Initialize sodium for the random generation below
    if (sodium_init() < 0) {
        return nullptr;
    }

    // Parse parameters:
    // Parse alpha
    std::vector<uint8_t> alpha_vec = from_hex(alpha_str);
    if (alpha_vec.size() != crypto_core_ristretto255_SCALARBYTES) {
        result = "{\"error\": \"Invalid alpha size\"}";
        return result.c_str();
    }
    unsigned char alpha[crypto_core_ristretto255_SCALARBYTES];
    memcpy(alpha, alpha_vec.data(), crypto_core_ristretto255_SCALARBYTES);

    // Parse overline_spk_i_start_G
    // std::cout << "[NEW] Parsing overline... " << overline_spk_i_start_G_str << std::endl;
    std::cout << "Parsing overline... " << std::endl;
    std::vector<uint8_t> g_vec = from_hex(overline_spk_i_start_G_str);
    if (g_vec.size() != crypto_core_ristretto255_BYTES) {
        result = "{\"error\": \"Invalid G size\"}";
        return result.c_str();
    }
    unsigned char overline_spk_i_start_G[crypto_core_ristretto255_BYTES];
    memcpy(overline_spk_i_start_G, g_vec.data(), crypto_core_ristretto255_BYTES);

    // Parse Tpp
    // std::cout << "[NEW] Parsing Tpp... " << Tpp_str << std::endl;
    std::cout << "Parsing Tpp... " << std::endl;
    std::vector<std::string> Tpp_tokens = extract_tokens(Tpp_str, ' ');
    if (Tpp_tokens.size() != ell) {
        result = "{\"error\": \"Tpp length mismatch\"}";
        return result.c_str();
    }
    unsigned char* Tpp[ell];
    for (size_t i = 0; i < ell; ++i) {
        std::vector<uint8_t> t = from_hex(Tpp_tokens.at(i));
        if (t.size() != crypto_core_ristretto255_BYTES) {
            result = "{\"error\": \"Tpp[" + std::to_string(i) + "] has wrong size\"}";
            return result.c_str();
        }
        Tpp[i] = new unsigned char[crypto_core_ristretto255_BYTES];
        memcpy(Tpp[i], t.data(), crypto_core_ristretto255_BYTES);
    }

    // Parse W_s
    //std::cout << "[NEW] Parsing Ws... " << W_s_str << std::endl;
    std::cout << "Parsing Ws... " << std::endl;
    std::vector<std::string> W_s_tokens = extract_tokens(W_s_str, ' ');
    if (W_s_tokens.size() != n_ballots) {
        result = "{\"error\": \"W_s length mismatch\"}";
        std::cout << "[NEW] Got inside if... ws_size: " << W_s_tokens.size() << " n_ballots: " << n_ballots << std::endl;
        return result.c_str();
    }

    std::cout << "[NEW] First loop... " << std::endl;
    unsigned char* W_s[n_ballots];
    for (size_t i = 0; i < n_ballots; ++i) {
        std::vector<uint8_t> w = from_hex(W_s_tokens.at(i));
        if (w.size() != crypto_core_ristretto255_BYTES) {
            result = "{\"error\": \"W_s[" + std::to_string(i) + "] has wrong size\"}";
            return result.c_str();
        }
        W_s[i] = new unsigned char[crypto_core_ristretto255_BYTES];
        memcpy(W_s[i], w.data(), crypto_core_ristretto255_BYTES);
    }

    /*std::cout << "[NEW] Second loop... - to remove " << std::endl;
    for (size_t i = 0; i < W_s_tokens.size(); i++) {
        std::cout << "W_s[" << i << "] = " << W_s_tokens.at(i)
                  << " addr: " << (void*)W_s[i] << std::endl;
    }*/

    // -----------------------------------------------------------------

    // G.IntersectVotes
    std::cout << "G.IntersectVotes..." << std::endl;
    auto intersect_start = std::chrono::high_resolution_clock::now();
    unsigned char inv_alpha[crypto_core_ristretto255_SCALARBYTES];
    crypto_core_ristretto255_scalar_invert(inv_alpha, alpha);
    unsigned char *T3p[ell];

    // c. Exponentiates Tpp to the inverse of alpha
    for (auto i = 0; i < ell; i++)
    {
        T3p[i] = (unsigned char *)malloc(crypto_core_ristretto255_BYTES);
        if (crypto_scalarmult_ristretto255(T3p[i], inv_alpha, Tpp[i]) != 0)
        {
            printf("could not exp to the 1/alpha exponentiation\n");
            abort();
        }
    }
    std::vector<int> X; // recovered votes
    unsigned char yw[crypto_core_ristretto255_BYTES];
    unsigned char recovered_yw[crypto_core_ristretto255_BYTES];
    unsigned char encoded_candidate_vote[crypto_core_ristretto255_SCALARBYTES];
    memset(encoded_candidate_vote, 0x00, crypto_core_ristretto255_SCALARBYTES);

    // c. Takes each tag
    for (auto y_indx = 0; y_indx < ell; y_indx++)
    {
        // c. Takes each encrypted ballot
        for (auto w_indx = 0; w_indx < n_ballots; w_indx++)
        {
            // c. For each possible value for a vote
            for (int candidate_vote = min_vote; candidate_vote <= max_vote; candidate_vote++)
            {
                encoded_candidate_vote[0] = (unsigned char)candidate_vote;
                crypto_core_ristretto255_add(yw, T3p[y_indx], W_s[w_indx]); // yw = T3p + W_s
                // due to a quirk in crypto_scalarmult_ristretto255, the 0-vote is handled separatedly
                if (candidate_vote == 0) {
                    ristretto255_identity_point(recovered_yw);
                } else {
                    // c. Compute b = a^k, crypto_scalarmult_ristretto255(b, k, a) != 0, if it's not the same, abort? TODO
                    if (crypto_scalarmult_ristretto255(recovered_yw, encoded_candidate_vote, overline_spk_i_start_G) != 0)
                    {
                        printf("failed to exp to the x in intersect votes\n");
                        printhex(encoded_candidate_vote, crypto_core_ristretto255_SCALARBYTES);
                        abort();
                    }
                }
                // if yw matches recovered_yw, we identified a vote
                if (memcmp(yw, recovered_yw, crypto_core_ristretto255_BYTES) == 0) {
                    X.push_back(candidate_vote);
                }
            }
        }
    }



    // Build JSON with permutation
    std::cout << "Printing result (X): ";
    result = "{";
    result += "\"X\": \"";
    for (size_t i = 0; i < X.size(); ++i) {
        result += std::to_string(X.at(i));
        std::cout << std::to_string(X.at(i)) << " ";
        if (i < ell - 1) result += " ";
    }
    result += "\"}";
    std::cout << "\n";

    // Cleanup
    for (size_t i = 0; i < ell; ++i) delete[] Tpp[i];
    for (size_t i = 0; i < n_ballots; ++i) delete[] W_s[i];

    std::cout << "Intersection finished successfully" << std::endl;

    return result.c_str(); // Emscript copies the result to JavaScript
}

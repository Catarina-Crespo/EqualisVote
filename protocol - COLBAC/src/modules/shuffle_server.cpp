//
// Created by Catarina on 24/06/2025.
//

#ifdef __cplusplus
extern "C"{
#endif
#include <sodium.h>
#ifdef __cplusplus
}
#endif

#include <cstdlib>
#include <string>
#include <cstring>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include "../dlog_to_gen.hpp"
#include "../parties.hpp"
#include "../batched_dleq.hpp"
#include "../verifiable_exponentiation.hpp"
#include "../utilities.hpp"
#include "../random_permutation.hpp"

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


const char* filename_in = "t.txt";
const char* filename_out = "tp.txt";
const char* filename_server_info = "init_server.txt";


// user_tokens is a map of: <user_public_key, user_group_token>
void shuffle_server(size_t ell, unsigned char **T, const unsigned char *Delta_i_star_G, const unsigned char *spk_i_star_G, size_t *rho_i_star_G) {

    std::cout << "S.ShuffleExp..." << std::endl;

    // c. Creates an array of pointers, where each Tprime[i] is a pointer to a raw byte array
    // c. The code here is allocating memory (crypto_core_ristretto255_BYTES) for each array
    unsigned char *Tprime[ell];
    for (auto i = 0; i < ell; i++) {
        Tprime[i] = (unsigned char *) malloc(crypto_core_ristretto255_BYTES);
    }

    // perform VPE evaluation
    ShuffledSigmaProtocol<SCDLEQSigmaProtocol> pfsys(ell);
    RepeatedSigmaProtocol<ShuffledSigmaProtocol<SCDLEQSigmaProtocol>> repfsys(128, pfsys);
    FiatShamir<RepeatedSigmaProtocol<ShuffledSigmaProtocol<SCDLEQSigmaProtocol>>> fsproof(repfsys);
    VEP<FiatShamir<RepeatedSigmaProtocol<ShuffledSigmaProtocol<SCDLEQSigmaProtocol>>>> vep(&fsproof);

    /*std::cout << "Calling vep.eval with ell = " << ell << std::endl;
    for (size_t i = 0; i < ell; ++i) {
        std::cout << "T[" << i << "] = " << to_hex(T[i], crypto_core_ristretto255_BYTES) << std::endl;
        std::cout << "rho[" << i << "] = " << rho_i_star_G[i] << std::endl;
    }
    std::cout << "Delta = " << to_hex(Delta_i_star_G, crypto_core_ristretto255_SCALARBYTES) << std::endl;
    std::cout << "SPK = " << to_hex(spk_i_star_G, crypto_core_ristretto255_BYTES) << std::endl;
    */

    unsigned char S_shuffle_exp_proof[vep.exponentiation_proof_BYTES];
    if (vep.eval(
        Delta_i_star_G, spk_i_star_G,
        ell, T,
        rho_i_star_G,
        Tprime,
        S_shuffle_exp_proof
    ) != 0) {
        for (size_t i = 0; i < ell; ++i) {
            std::cout << "Tprime[" << i << "] = " << to_hex(Tprime[i], crypto_core_ristretto255_BYTES) << std::endl;
        }
        assert(false);
    }

    // check proof
    assert(vep.check(spk_i_star_G, ell, T, Tprime, S_shuffle_exp_proof));

}

int main(int argc, char *argv[]) {

    // Initialize libsodium
    if (sodium_init() < 0) {
        std::cerr << "libsodium init failed" << std::endl;
        return 1;
    }

    // Reads T
    std::cout << "Starting to read file\n";
    std::ifstream in_file(filename_in);
    std::string line;
    std::cout << "Get line with tokens\n";
    std::getline(in_file, line);
    std::istringstream token_stream(line);

    std::vector<std::string> hex_tokens;
    std::string token;
    std::cout << "Split tokens\n";
    while (token_stream >> token) {
        hex_tokens.push_back(token);
    }
    size_t ell = hex_tokens.size();
    std::cout << "Starting to parse tokens\n";
    unsigned char** T = new unsigned char*[ell];
    for (size_t i = 0; i < ell; ++i) {
        std::vector<unsigned char> bytes = from_hex(hex_tokens[i]);
        T[i] = new unsigned char[crypto_core_ristretto255_BYTES];
        memcpy(T[i], bytes.data(), crypto_core_ristretto255_BYTES);
    }
    std::cout << "Finished parsing tokens\n";

    // delta - 3, spk - 4, rho - 6

    // --- Step 2: Read Delta_i_star_G, spk_i_star_G, rho_i_star_G from JSON ---
    std::ifstream server_info_file(filename_server_info);
    auto count = 0;
    unsigned char Delta_i_star_G[crypto_core_ristretto255_SCALARBYTES];
    unsigned char spk_i_star_G[crypto_core_ristretto255_BYTES];
    auto* rho_i_star_G = new size_t[ell];
    while (count < 7) {
        if (!std::getline(server_info_file, line)) {
            std::cerr << "Unexpected end of server_info_file at line " << count << "\n";
            break;
        }

        // Handle delta
        if (count == 3) {
            auto Delta_bytes = from_hex(line);
            if (Delta_bytes.size() != crypto_core_ristretto255_SCALARBYTES) {
                std::cerr << "Delta_bytes wrong size: " << Delta_bytes.size() << "\n";
            }
            memcpy(Delta_i_star_G, Delta_bytes.data(), crypto_core_ristretto255_SCALARBYTES);
        } else if (count == 4) {
            auto spk_bytes = from_hex(line);
            if (spk_bytes.size() != crypto_core_ristretto255_BYTES) {
                std::cerr << "Delta_bytes wrong size: " << spk_bytes.size() << "\n";
            }
            memcpy(spk_i_star_G, spk_bytes.data(), crypto_core_ristretto255_BYTES);
        } else if (count == 6) {
            std::istringstream stream(line);
            size_t val;
            size_t i = 0;
            while (stream >> val) {
                if (i >= ell) {
                    std::cerr << "Too many values for rho_i_star_G (expected " << ell << ")\n";
                    std::exit(1);
                }
                rho_i_star_G[i++] = val;
            }
            if (i != ell) {
                std::cerr << "Too few values for rho_i_star_G (got " << i << ", expected " << ell << ")\n";
                std::exit(1);
            }
        }
        count++;
    }


    // --- Step 3: Shuffle ---
    shuffle_server(ell, T, Delta_i_star_G, spk_i_star_G, rho_i_star_G);

    // --- Step 4: Write output Tprime to filename_out ---
    std::ofstream out_file(filename_out);
    for (size_t i = 0; i < ell; ++i) {
        out_file << to_hex(T[i], crypto_core_ristretto255_BYTES) << " ";
        delete[] T[i];
    }
    out_file << "\n";
    out_file.close();

    // Clean up
    delete[] T;
    delete[] rho_i_star_G;

    std::cout << "Tp produced\n";

    return 0;
}


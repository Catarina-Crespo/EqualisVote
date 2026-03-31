// init_count_jni.cpp
// JNI-adapted version of init_count()

#ifdef __cplusplus
extern "C" {
#endif
#include <sodium.h>
#ifdef __cplusplus
}
#endif

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <string>

#include "ShuffleResult.hpp"

#include "../dlog_to_gen.hpp"
#include "../parties.hpp"
#include "../batched_dleq.hpp"
#include "../verifiable_exponentiation.hpp"
#include "../utilities.hpp"
#include "../random_permutation.hpp"
#include "../fiat_shamir.hpp"
#include "../repeated_sigma_protocol.hpp"
#include "../shuffle_compatible_dleq.hpp"
#include "../shuffled_sigma_protocol.hpp"
#include "../base_point.hpp"

ShuffleResult init_count_jni(
    size_t group_size,
    const unsigned char *s_G,
    unsigned char **T,
    const std::vector<unsigned char *> &vec_of_ballots,
    size_t n_ballots
) {
    static bool sodium_ready = false;
    if (!sodium_ready) {
        if (sodium_init() < 0) abort();
        sodium_ready = true;
    }

    ShuffleResult result;
    size_t ell = group_size;

    /* MUST BE NULL, NOT nullptr */
    VEP<BatchedDLEQProof> ve(NULL);
    DLOG2GenProof dlog2gen_pf;

    // ------------------- Init Count -------------------

    size_t *rho_i_star_G = new size_t[ell];  // ← Use heap allocation
    random_permutation(ell, rho_i_star_G);

    unsigned char s_i_star_G[ve.secret_key_BYTES];
    unsigned char overline_spk_i_start_G[ve.public_key_BYTES];
    unsigned char overline_spk_proof[ve.key_proof_BYTES];

    ve.gen(s_i_star_G, overline_spk_i_start_G, overline_spk_proof);

    unsigned char inv_s_G[crypto_core_ristretto255_SCALARBYTES];
    if (crypto_core_ristretto255_scalar_invert(inv_s_G, s_G) != 0) {
        delete[] rho_i_star_G;
        abort();
    }

    unsigned char Delta_i_star_G[crypto_core_ristretto255_SCALARBYTES];
    unsigned char spk_i_star_G[crypto_core_ristretto255_BYTES];
    unsigned char spk_i_star_proof[DLOGProof::proof_BYTES];

    crypto_core_ristretto255_scalar_mul(Delta_i_star_G, s_i_star_G, inv_s_G);
    crypto_scalarmult_ristretto255_base(spk_i_star_G, Delta_i_star_G);

    dlog2gen_pf.prove(Delta_i_star_G, spk_i_star_G, spk_i_star_proof);

    assert(dlog2gen_pf.verify(spk_i_star_G, spk_i_star_proof));
    assert(dlog2gen_pf.verify(overline_spk_i_start_G, overline_spk_proof));

    // ------------------- Shuffle T -------------------
    // --- debug prints ---
    std::cerr << "[DEBUG] overline_spk_i_start_G: " << to_hex(overline_spk_i_start_G, ve.public_key_BYTES) << std::endl;
    std::cerr << "[DEBUG] T[0]: " << (ell > 0 ? to_hex(T[0], crypto_core_ristretto255_BYTES) : "") << std::endl;
    std::cerr << "[DEBUG] vec_of_ballots[0]: " << (n_ballots > 0 ? to_hex(vec_of_ballots[0], crypto_core_ristretto255_BYTES) : "") << std::endl;


    unsigned char **Tprime = new unsigned char*[ell];  // ← Use heap allocation
    for (size_t i = 0; i < ell; i++) {
        Tprime[i] = (unsigned char *) malloc(crypto_core_ristretto255_BYTES);
    }

    ShuffledSigmaProtocol<SCDLEQSigmaProtocol> pfsys(ell);
    RepeatedSigmaProtocol<ShuffledSigmaProtocol<SCDLEQSigmaProtocol>> repfsys(128, pfsys);
    FiatShamir<RepeatedSigmaProtocol<ShuffledSigmaProtocol<SCDLEQSigmaProtocol>>> fsproof(repfsys);
    VEP<FiatShamir<RepeatedSigmaProtocol<ShuffledSigmaProtocol<SCDLEQSigmaProtocol>>>> vep(&fsproof);

    unsigned char S_shuffle_exp_proof[vep.exponentiation_proof_BYTES];

    if (vep.eval(
            Delta_i_star_G,
            spk_i_star_G,
            ell,
            T,
            rho_i_star_G,
            Tprime,
            S_shuffle_exp_proof
        ) != 0) {
        // Clean up on error
        for (size_t i = 0; i < ell; ++i) {
            free(Tprime[i]);
        }
        delete[] Tprime;
        delete[] rho_i_star_G;
        abort();
    }

    assert(vep.check(spk_i_star_G, ell, T, Tprime, S_shuffle_exp_proof));

    // ------------------- Server Shuffle Ballots -------------------

    unsigned char **W = new unsigned char*[n_ballots];    // ← Use heap allocation
    unsigned char **W_s = new unsigned char*[n_ballots];  // ← Use heap allocation

    for (size_t i = 0; i < n_ballots; i++) {
        W[i]   = (unsigned char *) malloc(crypto_core_ristretto255_BYTES);
        W_s[i] = (unsigned char *) malloc(crypto_core_ristretto255_BYTES);
    }

    for (size_t i = 0; i < n_ballots; i++) {
        memcpy(W[i], vec_of_ballots[i], crypto_core_ristretto255_BYTES);
    }

    unsigned char S_send_votes_proof[ve.exponentiation_proof_BYTES];

    if (ve.eval(
            s_i_star_G,
            overline_spk_i_start_G,
            n_ballots,
            W,
            NULL,  // ← No permutation for ballots
            W_s,
            S_send_votes_proof
        ) != 0) {
        // Clean up on error
        for (size_t i = 0; i < ell; ++i) {
            free(Tprime[i]);
        }
        delete[] Tprime;
        for (size_t i = 0; i < n_ballots; ++i) {
            free(W[i]);
            free(W_s[i]);
        }
        delete[] W;
        delete[] W_s;
        delete[] rho_i_star_G;
        abort();
    }

    assert(ve.check(
        overline_spk_i_start_G,
        n_ballots,
        W,
        W_s,
        S_send_votes_proof
    ));

    // ------------------- Populate result -------------------

    result.s_i_star_G = to_hex(s_i_star_G, ve.secret_key_BYTES);
    result.overline_spk_i_start_G = to_hex(overline_spk_i_start_G, ve.public_key_BYTES);
    result.overline_spk_proof = to_hex(overline_spk_proof, ve.key_proof_BYTES);
    result.Delta_i_star_G = to_hex(Delta_i_star_G, crypto_core_ristretto255_SCALARBYTES);
    result.spk_i_star_G = to_hex(spk_i_star_G, crypto_core_ristretto255_BYTES);
    result.spk_i_star_proof = to_hex(spk_i_star_proof, DLOGProof::proof_BYTES);

    // Build strings BEFORE freeing memory
    for (size_t i = 0; i < ell; ++i) {
        result.rho_i_star_G += std::to_string(rho_i_star_G[i]);
        if (i + 1 < ell) result.rho_i_star_G += " ";
    }

    for (size_t i = 0; i < ell; ++i) {  // ← Use ell consistently
        result.Tprime += to_hex(Tprime[i], crypto_core_ristretto255_BYTES);
        if (i + 1 < ell) result.Tprime += " ";
    }

    for (size_t i = 0; i < n_ballots; ++i) {
        result.W_s += to_hex(W_s[i], crypto_core_ristretto255_BYTES);
        if (i + 1 < n_ballots) result.W_s += " ";
    }

    // --- debug prints ---
    std::cerr << "[DEBUG] overline_spk_i_start_G: " << result.overline_spk_i_start_G << std::endl;
    std::cerr << "[DEBUG] Tprime[0]: " << (ell > 0 ? to_hex(Tprime[0], crypto_core_ristretto255_BYTES) : "") << std::endl;
    std::cerr << "[DEBUG] W_s[0]: " << (n_ballots > 0 ? to_hex(W_s[0], crypto_core_ristretto255_BYTES) : "") << std::endl;


    // ------------------- Cleanup -------------------
    for (size_t i = 0; i < ell; ++i) {
        free(Tprime[i]);
    }
    delete[] Tprime;
    delete[] rho_i_star_G;

    for (size_t i = 0; i < n_ballots; ++i) {
        free(W[i]);
        free(W_s[i]);
    }
    delete[] W;
    delete[] W_s;

    return result;
}
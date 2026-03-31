#pragma once

#ifdef __cplusplus
extern "C"{
#endif
#include <sodium.h>
#ifdef __cplusplus
}
#endif

#include "sigma_protocol.hpp"

/*
    This proof offers 2^{-1} soundness error, meaning that it would need to be repeated lambda times to achieve 2^{-lambda} soundness error.
*/

class SCDLEQSigmaProtocol : public ISigmaProtocol
{
public:
    static const size_t witness_BYTES = crypto_core_ristretto255_SCALARBYTES; // w
    static const size_t statement_BYTES = 4 * crypto_core_ristretto255_BYTES; // (g, h, g^w, h^w)
    static const size_t state_BYTES = crypto_core_ristretto255_SCALARBYTES; // alpha
    static const size_t commitment_BYTES = 2 * crypto_core_ristretto255_BYTES; // (g^alpha, w^alpha)
    static const size_t challenge_BYTES = 1; // e \in {0, 1}
    static const size_t response_BYTES = crypto_core_ristretto255_SCALARBYTES; // z
    int prover_commitment(
        const unsigned char (*witness),
        const unsigned char (*statement),
        unsigned char (*state),
        unsigned char (*commitment)
    );
    void verifier_challenge(
        const unsigned char (*statement),
        const unsigned char (*commitment),
        unsigned char (*challenge)
    );
    int prover_response(
        const unsigned char (*witness),
        const unsigned char (*statement),
        const unsigned char (*state),
        const unsigned char (*commitment),
        const unsigned char (*challenge),
        unsigned char (*response)
    );
    bool verifier_check(
        const unsigned char (*statement),
        const unsigned char (*commitment),
        const unsigned char (*challenge),
        const unsigned char (*response)
    );
};

void test_shuffle_compatible_DLEQ_SigmaProtocol();

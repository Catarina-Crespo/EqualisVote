#pragma once

#ifdef __cplusplus
extern "C"{
#endif
#include <sodium.h>
#ifdef __cplusplus
}
#endif

#include "sigma_protocol.hpp"

template <class SCSigmaProtocol>
class ShuffledSigmaProtocol : public ISigmaProtocol
{
public:
    size_t witness_BYTES; // (x_j)_j, pi
    size_t statement_BYTES; // (c0_j)_j, (c1_j)_j
    size_t state_BYTES; // (alpha_j)_j, pi_a
    size_t commitment_BYTES; // (a_{pi_a(j)})_j
    static const size_t challenge_BYTES = 1; // e \in {0, 1}
    size_t response_BYTES; // (z_{pi_a(j)})_j, pi_z
    size_t elements;

    ShuffledSigmaProtocol(size_t n) : elements(n)
    {
        size_t permutation_BYTES = sizeof(size_t) * n;
        witness_BYTES = n * SCSigmaProtocol::witness_BYTES + permutation_BYTES;
        statement_BYTES = n * SCSigmaProtocol::statement_BYTES;
        state_BYTES = n * SCSigmaProtocol::state_BYTES + permutation_BYTES;
        commitment_BYTES = n * SCSigmaProtocol::commitment_BYTES;
        response_BYTES = n * SCSigmaProtocol::response_BYTES + permutation_BYTES;
    };
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

template <class SCSigmaProtocol> void test_ShuffledSigmaProtocol_high_error(size_t n);

#pragma once

#ifdef __cplusplus
extern "C"{
#endif
#include <sodium.h>
#ifdef __cplusplus
}
#endif

#include "sigma_protocol.hpp"

template <class SigmaProtocol>
class RepeatedSigmaProtocol : public ISigmaProtocol
{
public:
    size_t witness_BYTES; // (x_j)_j, pi
    size_t statement_BYTES; // (c0_j)_j, (c1_j)_j
    size_t state_BYTES; // (alpha_j)_j, pi_a
    size_t commitment_BYTES; // (a_{pi_a(j)})_j
    size_t challenge_BYTES; // e \in {0, 1}
    size_t response_BYTES; // (z_{pi_a(j)})_j, pi_z
    size_t repetitions;
    SigmaProtocol *inner_pfsys;

    RepeatedSigmaProtocol(size_t t, SigmaProtocol &_inner_pfsys) : repetitions(t)
    {
        inner_pfsys = &_inner_pfsys;
        witness_BYTES = _inner_pfsys.witness_BYTES;
        statement_BYTES = _inner_pfsys.statement_BYTES;
        state_BYTES = t * _inner_pfsys.state_BYTES;
        commitment_BYTES = t * _inner_pfsys.commitment_BYTES;
        challenge_BYTES = t * _inner_pfsys.challenge_BYTES;
        response_BYTES = t * _inner_pfsys.response_BYTES;
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

template <class SigmaProtocol, class ShuffledSigmaProtocol> void test_RepeatedSigmaProtocol(size_t t, size_t n);

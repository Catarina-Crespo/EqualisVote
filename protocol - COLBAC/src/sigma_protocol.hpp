#pragma once 

#include <cstddef>

class ISigmaProtocol
{
public:
    const static size_t witness_BYTES;
    const static size_t statement_BYTES;
    const static size_t state_BYTES;
    const static size_t commitment_BYTES;
    const static size_t challenge_BYTES;
    const static size_t response_BYTES;
    virtual int prover_commitment(
        const unsigned char (*witness),
        const unsigned char (*statement),
        unsigned char (*state),
        unsigned char (*commitment)
    ) = 0;
    virtual void verifier_challenge(
        const unsigned char (*statement),
        const unsigned char (*commitment),
        unsigned char (*challenge)
    ) = 0;
    virtual int prover_response(
        const unsigned char (*witness),
        const unsigned char (*statement),
        const unsigned char (*state),
        const unsigned char (*commitment),
        const unsigned char (*challenge),
        unsigned char (*response)
    ) = 0;
    virtual bool verifier_check(
        const unsigned char (*statement),
        const unsigned char (*commitment),
        const unsigned char (*challenge),
        const unsigned char (*response)
    ) = 0;
};

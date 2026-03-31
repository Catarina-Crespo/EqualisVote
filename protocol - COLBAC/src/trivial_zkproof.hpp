#pragma once

#ifdef __cplusplus
extern "C"{
#endif
#include <sodium.h>
#ifdef __cplusplus
}
#endif

#include "zkproof.hpp"

class TrivialProof : public IZKProof
{
    public:
    static const size_t witness_BYTES = 0;
    static const size_t statement_BYTES = 0;
    static const size_t proof_BYTES = 0;
    int prove(
        const unsigned char (*witness),
        const unsigned char (*statement),
        unsigned char (*proof)
    );
    bool verify(
        const unsigned char (*statement),
        const unsigned char (*proof)
    );
};

void test_TrivialProof();
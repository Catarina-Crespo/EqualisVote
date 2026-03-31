#pragma once

#ifdef __cplusplus
extern "C"{
#endif
#include <sodium.h>
#ifdef __cplusplus
}
#endif

#include "zkproof.hpp"
#include <cassert>

class BatchedDLEQProof : public IZKProof
{
    private:
    static const size_t salt_BYTES = crypto_hash_sha256_BYTES;
    size_t commitment_BYTES; // (g^t, a_1^t, ..., a_{n-1}^t)
    public:
    size_t elements;
    static const size_t witness_BYTES = crypto_core_ristretto255_SCALARBYTES; // w
    size_t statement_BYTES; // (g, a_1, ..., a_{n-1}, g^w, a_1^w, ...., a_{n-1}^w)
    static const size_t proof_BYTES = crypto_hash_sha256_BYTES  // salt, used to separate random oracles
        + crypto_core_ristretto255_SCALARBYTES                  // challenge
        + crypto_core_ristretto255_SCALARBYTES;                 // response
    BatchedDLEQProof(size_t n) :
        elements(n),
        commitment_BYTES(n * crypto_core_ristretto255_BYTES),
        statement_BYTES(2 * n * crypto_core_ristretto255_BYTES)
        {
            assert(n > 0);
        };
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

void test_BatchedDLEQProof(size_t n);
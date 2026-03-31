#pragma once

#ifdef __cplusplus
extern "C"{
#endif
#include <sodium.h>
#ifdef __cplusplus
}
#endif

#include "zkproof.hpp"

class DLOGProof : public IZKProof
{
    private:
    static const size_t salt_BYTES = crypto_hash_sha256_BYTES;
    static const size_t commitment_BYTES = crypto_core_ristretto255_BYTES;
    public:
    static const size_t witness_BYTES = crypto_core_ristretto255_SCALARBYTES; // w
    static const size_t statement_BYTES = 2 * crypto_core_ristretto255_BYTES; // (g, g^w)
    static const size_t proof_BYTES = crypto_hash_sha256_BYTES  // salt, used to separate random oracles
        + crypto_core_ristretto255_SCALARBYTES                  // challenge
        + crypto_core_ristretto255_SCALARBYTES;                 // response
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

void test_DLOGProof();
#pragma once

#ifdef __cplusplus
extern "C"{
#endif
#include <sodium.h>
#ifdef __cplusplus
}
#endif

#include "zkproof.hpp"

template <class SigmaProtocol>
class FiatShamir : public IZKProof
{
    private:
    static const size_t salt_BYTES = crypto_hash_sha256_BYTES;
    // static const size_t commitment_BYTES = SigmaProtocol::commitment_BYTES;
    public:
    size_t witness_BYTES; // w
    size_t statement_BYTES; // = SigmaProtocol::statement_BYTES; // (g, a, g^w, a^w)
    size_t proof_BYTES; // = crypto_hash_sha256_BYTES  // salt, used to separate random oracles
        // + SigmaProtocol::commitment_BYTES                       // commitment
        // + SigmaProtocol::response_BYTES;                        // response
    SigmaProtocol *inner_pfsys;
    FiatShamir(SigmaProtocol &_inner_pfsys)
    {
        inner_pfsys = &_inner_pfsys;
        witness_BYTES = _inner_pfsys.witness_BYTES;
        statement_BYTES = _inner_pfsys.statement_BYTES;
        proof_BYTES = crypto_hash_sha256_BYTES + _inner_pfsys.commitment_BYTES + _inner_pfsys.response_BYTES;
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

template<class SigmaProtocol> void test_FiatShamir_SCDLEQ();
template<class SCSigmaProtocol, class ShuffledSCSigmaProtocol> void test_FiatShamir_RepeatedShuffledDLEQ(size_t t, size_t n);
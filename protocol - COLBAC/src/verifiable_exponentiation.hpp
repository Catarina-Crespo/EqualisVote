#pragma once

#ifdef __cplusplus
extern "C" {
#endif
#include <sodium.h>
#ifdef __cplusplus
}
#endif

#include "dlog.hpp"
#include "batched_dleq.hpp"

template<class ZKProofSystem>
class VEP {
public:
    const static size_t group_element_BYTES = crypto_core_ristretto255_BYTES;
    const static size_t secret_key_BYTES = crypto_core_ristretto255_SCALARBYTES;
    const static size_t public_key_BYTES = crypto_core_ristretto255_BYTES;
    const static size_t key_proof_BYTES = DLOGProof::proof_BYTES;
    size_t exponentiation_proof_BYTES;
    bool shuffled;
    ZKProofSystem *pfsys;

    VEP(ZKProofSystem *_pfsys) {
        pfsys = _pfsys;
        if (pfsys == NULL) {
            shuffled = false;
            // exponentiation_proof_BYTES = ZKProofSystem::proof_BYTES;
        } else {
            shuffled = true;
            // assert(false);
        }
        exponentiation_proof_BYTES = pfsys->proof_BYTES;
        // shuffled = _shuffled;
        // if (shuffled) {
        //     exponentiation_proof_BYTES = 0; // TODO
        //     assert(false);
        // } else {
        //     exponentiation_proof_BYTES = BatchedDLEQProof::proof_BYTES;
        // }
    };

    int gen(
        unsigned char (*secret_key),
        unsigned char (*public_key),
        unsigned char (*key_proof)
    );

    int eval(
        const unsigned char (*secret_key),
        const unsigned char (*public_key),
        const size_t n_of_group_elements,
        unsigned char (**group_elements),
        size_t *shuffle,
        unsigned char (**exponentiated_group_elements),
        unsigned char (*proof)
    );

    bool check(
        const unsigned char *public_key,
        const size_t n_of_group_elements,
        unsigned char (**group_elements),
        unsigned char (**exponentiated_group_elements),
        unsigned char (*proof)
    );

    // -------------------------
    int evalSets(
        const unsigned char (*secret_key),
        const unsigned char (*public_key),
        size_t n_of_subsets,
        size_t n_tags_per_subset,
        unsigned char (***tag_subsets),
        size_t *shuffle,
        unsigned char (***exponentiated_tag_subsets),
        unsigned char (*proof)
    );
};

template<class ZKProofSystem>
void test_VE(size_t n);

template<class SCSigmaProtocol, class ShuffledSCSigmaProtocol>
void test_VEP(size_t t, size_t n);

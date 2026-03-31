#include <cassert>
#include <cstring>
#include <chrono>
#include <iostream>
#include "utilities.hpp"
#include "base_point.hpp"
#include "verifiable_exponentiation.hpp"
#include "dlog_to_gen.hpp"
#include "repeated_sigma_protocol.hpp"
#include "fiat_shamir.hpp"
#include "random_permutation.hpp"

/*
VE(P) functionality, cf. Figure 7
*/

// returns 0 if prove succeeded, -1 otherwise
template<class ZKProofSystem>
int VEP<ZKProofSystem>::gen(
    unsigned char (*secret_key),
    unsigned char (*public_key),
    unsigned char (*key_proof)
) {
    assert(DLOG2GenProof::witness_BYTES == secret_key_BYTES);
    assert(DLOG2GenProof::statement_BYTES == public_key_BYTES);
    assert(DLOG2GenProof::proof_BYTES == key_proof_BYTES);

    // sample secret key
    crypto_core_ristretto255_scalar_random(secret_key);

    // generate public key
    crypto_scalarmult_ristretto255_base(public_key, secret_key);

    DLOG2GenProof pfsys;
    int rv = pfsys.prove(secret_key, public_key, key_proof);

    return rv;
}

template<class ZKProofSystem>
int VEP<ZKProofSystem>::eval(
    const unsigned char (*secret_key),
    const unsigned char (*public_key),
    const size_t n_of_group_elements,
    unsigned char (**group_elements),
    size_t *shuffle, // NULL if no shuffle applied
    unsigned char (**exponentiated_group_elements),
    unsigned char (*proof)
) {
    if (shuffled) {
        // VEP
        assert(shuffle != NULL);

        ZKProofSystem fsproof = *pfsys;

        unsigned char witness[fsproof.witness_BYTES];       // claim the prover wants to prove
        unsigned char statement[fsproof.statement_BYTES];   // secret information that proves the statement is true

        // prepare statement for Shuffled DLEQ
        unsigned char ordered_witness[fsproof.witness_BYTES];
        size_t permutation_BYTES = n_of_group_elements * sizeof(size_t);
        // TODO - what is this doing? Copies the shuffle to (witness + fsproof.witness_BYTES - permutation_BYTES)?
        memcpy((size_t *) (witness + fsproof.witness_BYTES - permutation_BYTES), shuffle, permutation_BYTES);

        // inner protocol
        // TODO - why three statements?
        unsigned char *statements[n_of_group_elements];
        unsigned char *statement0[n_of_group_elements];
        unsigned char *statement1[n_of_group_elements];

        /*
        DLEQ statement structure
        ========================

        statement[0] = (g1, a1, g1w1, a3w3)
        statement[1] = (g2, a2, g2w2, a3w3)
        statement[2] = (g3, a3, g2w2, a3w3)

        ShuffledDLEQ structure
        ======================

        A = pi(1), B = pi(2), C = pi(3)

        statement = (g1, a1, gAwA, aAwA),
                                         (g2, a2, gBwB, aBwB),
                                                              (g3, a3, gCwC, aCwC)
        */

        for (auto i = 0; i < n_of_group_elements; i++) {
            // create the witness
            memcpy(ordered_witness + i * secret_key_BYTES, secret_key, secret_key_BYTES);
            // crypto_core_ristretto255_scalar_random(ordered_witness + i * SCSigmaProtocol::witness_BYTES);

            // create the statement
            statements[i] = (unsigned char *) malloc(4 * group_element_BYTES);
            unsigned char *g = statements[i];
            unsigned char *a = g + group_element_BYTES;
            unsigned char *g_witness = a + group_element_BYTES;
            unsigned char *a_witness = g_witness + group_element_BYTES;
            statement0[i] = g;
            statement1[i] = g_witness;

            ristretto255_base_point(g);
            memcpy(a, group_elements[i], group_element_BYTES);
            // crypto_core_ristretto255_random(a);
            memcpy(g_witness, public_key, public_key_BYTES);
            // if (crypto_scalarmult_ristretto255(g_witness, ordered_witness + i * SCSigmaProtocol::witness_BYTES, g) != 0) {
            //     assert(false);
            // }

            // c. b = a^k : a_witness = group_elements[i]^secret_key
            if (crypto_scalarmult_ristretto255(a_witness, secret_key, group_elements[i]) != 0) {
                return -1;
            }
            // if (crypto_scalarmult_ristretto255(a_witness, ordered_witness + i * SCSigmaProtocol::witness_BYTES, a) != 0) {
            //     assert(false);
            // }
        }

        // pack individual exponentiations as single shuffled statement
        for (auto i = 0; i < n_of_group_elements; i++) {
            size_t shuffled_i = shuffle[i];

            // position (gi, ai)
            memcpy(
                statement + i * 4 * group_element_BYTES,
                statement0[i],
                2 * group_element_BYTES
            );

            // let j = pi(i)
            // position (gj^wj, aj^wj)
            memcpy(
                statement + 2 * group_element_BYTES + shuffled_i * 4 * group_element_BYTES,
                statement1[i],
                2 * group_element_BYTES
            );

            // copy out the shuffled-an-exponentiated group elements
            memcpy(exponentiated_group_elements[shuffled_i], statement1[i] + group_element_BYTES, group_element_BYTES);

            // // shuffle the witnessess too (unneeded in VEP since they are all the same)
            memcpy(
                witness + shuffled_i * secret_key_BYTES,
                ordered_witness + i * secret_key_BYTES,
                secret_key_BYTES
            );
        }

        for (auto i = 0; i < n_of_group_elements; i++) {
            free(statements[i]);
        }
        // prove the shuffled exponentiation
        if (fsproof.prove(witness, statement, proof) != 0) {
            return -1;
        }

        // printf("-- proof --\n");
        // printf("    valid: %d\n", fsproof.verify(statement, proof));
        // printf("  witness: "); printhex(witness, fsproof.witness_BYTES);
        // printf("statement: "); printhex(statement, fsproof.statement_BYTES);
        // printf("    proof: "); printhex(proof, fsproof.proof_BYTES);

        return 0;
    } else {
        // VE
        assert(shuffle == NULL);

        // printf("       pk: "); printhex((unsigned char *)public_key, group_element_BYTES);
        // compute the exponentiation
        for (auto i = 0; i < n_of_group_elements; i++) {
            if (crypto_scalarmult_ristretto255(
                    exponentiated_group_elements[i],
                    secret_key,
                    group_elements[i]) != 0) {
                return -1;
            }
            // printf("      exp: "); printhex(group_elements[i], group_element_BYTES);
            // printf("           "); printhex(exponentiated_group_elements[i], group_element_BYTES);
        }

        // prepare to prove the exponentiation
        BatchedDLEQProof dleqpfsys(n_of_group_elements + 1); // "+ 1" due to the public key
        assert(2 * (n_of_group_elements + 1) * group_element_BYTES == dleqpfsys.statement_BYTES);
        unsigned char *statement = (unsigned char *) malloc(dleqpfsys.statement_BYTES);

        // prepare statement as (g, a1, ..., an, g^w, a1^w, ..., an^w)
        unsigned char *g = statement;
        unsigned char *g_witness = g + (n_of_group_elements + 1) * group_element_BYTES;

        ristretto255_base_point(g);
        memcpy(g_witness, public_key, group_element_BYTES);
        for (auto i = 0; i < n_of_group_elements; i++) {
            memcpy(g + (i + 1) * group_element_BYTES, group_elements[i], group_element_BYTES);
            memcpy(g_witness + (i + 1) * group_element_BYTES, exponentiated_group_elements[i], group_element_BYTES);
            // printf("     _exp: "); printhex(g + (i + 1) * group_element_BYTES, group_element_BYTES);
            // printf("           "); printhex(g_witness + (i + 1) * group_element_BYTES, group_element_BYTES);
        }

        // printf("       sk: "); printhex((unsigned char *)(secret_key), VEP::secret_key_BYTES);
        // printf("statement: "); printhex(g, dleqpfsys.statement_BYTES/2);
        // printf("           "); printhex(g_witness, dleqpfsys.statement_BYTES/2);

        // prove the exponentiation
        int rv = dleqpfsys.prove(secret_key, statement, proof);
        free(statement);
        return rv;
    }

    return -1;
}


// this is a simple wrapper around BatchedDLEQ.verify
template<class ZKProofSystem>
bool VEP<ZKProofSystem>::check(
    const unsigned char *public_key,
    const size_t n_of_group_elements,
    unsigned char (**group_elements),
    unsigned char (**exponentiated_group_elements),
    unsigned char (*proof)
) {
    if (shuffled) {
        // VEP
        bool valid;

        ZKProofSystem fsproof = *pfsys;

        unsigned char statement[fsproof.statement_BYTES];

        /*
        DLEQ statement structure
        ========================

        statement[0] = (g1, a1, g1w1, a3w3)
        statement[1] = (g2, a2, g2w2, a3w3)
        statement[2] = (g3, a3, g2w2, a3w3)

        ShuffledDLEQ structure
        ======================

        A = pi(1), B = pi(2), C = pi(3)

        statement = (g1, a1, gAwA, aAwA),
                                         (g2, a2, gBwB, aBwB),
                                                              (g3, a3, gCwC, aCwC)
        */

        // for (auto i = 0; i < n_of_group_elements; i++)
        // {

        //     // create the statement
        //     statements[i] = (unsigned char *)malloc(4 * group_element_BYTES);
        //     unsigned char *g = statements[i];
        //     unsigned char *a = g + group_element_BYTES;
        //     unsigned char *g_witness = a + group_element_BYTES;
        //     unsigned char *a_witness = g_witness + group_element_BYTES;
        //     statement0[i] = g;
        //     statement1[i] = g_witness;

        //     ristretto255_base_point(g);
        //     memcpy(a, group_elements[i], group_element_BYTES);
        //     // crypto_core_ristretto255_random(a);
        //     memcpy(g_witness, public_key, public_key_BYTES);
        //     // if (crypto_scalarmult_ristretto255(g_witness, ordered_witness + i * SCSigmaProtocol::witness_BYTES, g) != 0) {
        //     //     assert(false);
        //     // }
        //     if (crypto_scalarmult_ristretto255(a_witness, secret_key, group_elements[i]) != 0)
        //     {
        //         return -1;
        //     }
        //     // if (crypto_scalarmult_ristretto255(a_witness, ordered_witness + i * SCSigmaProtocol::witness_BYTES, a) != 0) {
        //     //     assert(false);
        //     // }
        // }

        // pack individual exponentiations as single shuffled statement
        for (auto i = 0; i < n_of_group_elements; i++) {
            unsigned char *g = statement + i * 4 * group_element_BYTES;
            unsigned char *a = g + group_element_BYTES;
            unsigned char *g_witness_shuffled = a + group_element_BYTES;
            unsigned char *a_witness_shuffled = g_witness_shuffled + group_element_BYTES;

            // position (gi, ai)
            ristretto255_base_point(g);
            memcpy(
                a,
                group_elements[i],
                group_element_BYTES
            );

            // let j = pi(i)
            // position (gj^wj, aj^wj)
            memcpy(g_witness_shuffled, public_key, public_key_BYTES);
            memcpy(
                a_witness_shuffled,
                exponentiated_group_elements[i],
                group_element_BYTES
            );

            // copy out the shuffled-an-exponentiated group elements
            // memcpy(exponentiated_group_elements[shuffled_i], statement1[i] + group_element_BYTES, group_element_BYTES);

            // // shuffle the witnessess too (unneeded in VEP since they are all the same)
            // memcpy(
            //     witness + shuffled_i * SCSigmaProtocol::witness_BYTES,
            //     ordered_witness + i * SCSigmaProtocol::witness_BYTES,
            //     SCSigmaProtocol::witness_BYTES
            // );
        }

        valid = fsproof.verify(statement, proof);

        // printf("-- verify --\n");
        // printf("    valid: %d\n", valid);
        // printf("statement: "); printhex(statement, fsproof.statement_BYTES);
        // printf("    proof: "); printhex(proof, fsproof.proof_BYTES);

        return valid;
    } else {
        BatchedDLEQProof dleqpfsys(n_of_group_elements + 1); // "+ 1" due to the public key
        assert(2 * (n_of_group_elements + 1) * group_element_BYTES == dleqpfsys.statement_BYTES);
        unsigned char *statement = (unsigned char *) malloc(dleqpfsys.statement_BYTES);

        // prepare statement as (g, a1, ..., an, g^w, a1^w, ..., an^w)
        unsigned char *g = statement;
        unsigned char *g_witness = g + (n_of_group_elements + 1) * group_element_BYTES;

        ristretto255_base_point(g);
        memcpy(g_witness, public_key, group_element_BYTES);
        for (auto i = 0; i < n_of_group_elements; i++) {
            memcpy(g + (i + 1) * group_element_BYTES, group_elements[i], group_element_BYTES);
            memcpy(g_witness + (i + 1) * group_element_BYTES, exponentiated_group_elements[i], group_element_BYTES);
        }

        bool valid = dleqpfsys.verify(statement, proof);
        free(statement);
        return valid;
    }
    return false;
}

// ----------------------------- Modified version

template<class ZKProofSystem>
int VEP<ZKProofSystem>::evalSets(
    const unsigned char (*secret_key),
    const unsigned char (*public_key),
    const size_t n_of_subsets,
    const size_t n_tags_per_subset,
    unsigned char (***tag_subsets), // tag_subsets[i][j] is j-th tag in i-th subset
    size_t *shuffle, // shuffle[i] tells where to place subset i
    unsigned char (***exponentiated_tag_subsets),
    unsigned char (*proof)
) {
    size_t total_tags = n_of_subsets * n_tags_per_subset;

    if (shuffled) {
        // VEP
        assert(shuffle != NULL);

        ZKProofSystem fsproof = *pfsys;

        unsigned char witness[fsproof.witness_BYTES];
        unsigned char statement[fsproof.statement_BYTES];

        // prepare statement for Shuffled DLEQ
        unsigned char ordered_witness[fsproof.witness_BYTES];
        size_t permutation_BYTES = n_of_subsets * sizeof(size_t);
        memcpy((size_t *) (witness + fsproof.witness_BYTES - permutation_BYTES), shuffle, permutation_BYTES);

        // inner protocol
        unsigned char *statements[total_tags];
        unsigned char *statement0[total_tags];
        unsigned char *statement1[total_tags];

        /*
        DLEQ statement structure
        ========================

        statement[0] = (g1, a1, g1w1, a3w3)
        statement[1] = (g2, a2, g2w2, a3w3)
        statement[2] = (g3, a3, g2w2, a3w3)

        ShuffledDLEQ structure
        ======================

        A = pi(1), B = pi(2), C = pi(3)

        statement = (g1, a1, gAwA, aAwA),
                                         (g2, a2, gBwB, aBwB),
                                                              (g3, a3, gCwC, aCwC)
        */

        // Same for loop as original version
        for (size_t i = 0; i < n_of_subsets; ++i) {
            size_t shuffled_i = shuffle[i];

            for (size_t j = 0; j < n_tags_per_subset; ++j) {
                size_t flat_index = i * n_tags_per_subset + j;
                size_t shuffled_flat_index = shuffled_i * n_tags_per_subset + j;

                // Prepare witness
                memcpy(ordered_witness + flat_index * secret_key_BYTES, secret_key, secret_key_BYTES);

                // Allocate and construct statement
                statements[flat_index] = (unsigned char *)malloc(4 * group_element_BYTES);
                unsigned char *g = statements[flat_index];
                unsigned char *a = g + group_element_BYTES;
                unsigned char *g_witness = a + group_element_BYTES;
                unsigned char *a_witness = g_witness + group_element_BYTES;

                statement0[flat_index] = g;
                statement1[flat_index] = g_witness;

                ristretto255_base_point(g);
                memcpy(a, tag_subsets[i][j], group_element_BYTES);
                memcpy(g_witness, public_key, group_element_BYTES);

                if (crypto_scalarmult_ristretto255(a_witness, secret_key, tag_subsets[i][j]) != 0) {
                    return -1;
                }

                // Store exponentiated tag in shuffled position
                memcpy(exponentiated_tag_subsets[shuffled_i][j], a_witness, group_element_BYTES);

                // Store witness in correct shuffled index
                memcpy(witness + shuffled_flat_index * secret_key_BYTES,
                       ordered_witness + flat_index * secret_key_BYTES,
                       secret_key_BYTES);

                // Pack into statement
                memcpy(statement + flat_index * 4 * group_element_BYTES,
                       g, 2 * group_element_BYTES);

                memcpy(statement + 2 * group_element_BYTES + shuffled_flat_index * 4 * group_element_BYTES,
                       g_witness, 2 * group_element_BYTES);
            }
        }

        for (size_t i = 0; i < total_tags; ++i) {
            free(statements[i]);
        }

        // Final proof
        if (fsproof.prove(witness, statement, proof) != 0) {
            return -1;
        }

        return 0;
        // Then else

        /*for (auto i = 0; i < n_of_group_elements; i++) {
            // create the witness
            memcpy(ordered_witness + i * secret_key_BYTES, secret_key, secret_key_BYTES);
            // crypto_core_ristretto255_scalar_random(ordered_witness + i * SCSigmaProtocol::witness_BYTES);

            // create the statement
            statements[i] = (unsigned char *) malloc(4 * group_element_BYTES);
            unsigned char *g = statements[i];
            unsigned char *a = g + group_element_BYTES;
            unsigned char *g_witness = a + group_element_BYTES;
            unsigned char *a_witness = g_witness + group_element_BYTES;
            statement0[i] = g;
            statement1[i] = g_witness;

            ristretto255_base_point(g);
            memcpy(a, group_elements[i], group_element_BYTES);
            // crypto_core_ristretto255_random(a);
            memcpy(g_witness, public_key, public_key_BYTES);
            // if (crypto_scalarmult_ristretto255(g_witness, ordered_witness + i * SCSigmaProtocol::witness_BYTES, g) != 0) {
            //     assert(false);
            // }

            // c. b = a^k : a_witness = group_elements[i]^secret_key
            if (crypto_scalarmult_ristretto255(a_witness, secret_key, group_elements[i]) != 0) {
                return -1;
            }
            // if (crypto_scalarmult_ristretto255(a_witness, ordered_witness + i * SCSigmaProtocol::witness_BYTES, a) != 0) {
            //     assert(false);
            // }
        }

        // pack individual exponentiations as single shuffled statement
        for (auto i = 0; i < n_of_group_elements; i++) {
            size_t shuffled_i = shuffle[i];

            // position (gi, ai)
            memcpy(
                statement + i * 4 * group_element_BYTES,
                statement0[i],
                2 * group_element_BYTES
            );

            // let j = pi(i)
            // position (gj^wj, aj^wj)
            memcpy(
                statement + 2 * group_element_BYTES + shuffled_i * 4 * group_element_BYTES,
                statement1[i],
                2 * group_element_BYTES
            );

            // copy out the shuffled-an-exponentiated group elements
            memcpy(exponentiated_group_elements[shuffled_i], statement1[i] + group_element_BYTES, group_element_BYTES);

            // // shuffle the witnessess too (unneeded in VEP since they are all the same)
            memcpy(
                witness + shuffled_i * secret_key_BYTES,
                ordered_witness + i * secret_key_BYTES,
                secret_key_BYTES
            );
        }

        for (auto i = 0; i < n_of_group_elements; i++) {
            free(statements[i]);
        }
        // prove the shuffled exponentiation
        if (fsproof.prove(witness, statement, proof) != 0) {
            return -1;
        }

        // printf("-- proof --\n");
        // printf("    valid: %d\n", fsproof.verify(statement, proof));
        // printf("  witness: "); printhex(witness, fsproof.witness_BYTES);
        // printf("statement: "); printhex(statement, fsproof.statement_BYTES);
        // printf("    proof: "); printhex(proof, fsproof.proof_BYTES);
*/
        return 0;
    } else {
        // VE
        assert(shuffle == NULL);

        // printf("       pk: "); printhex((unsigned char *)public_key, group_element_BYTES);
        // compute the exponentiation
        /*for (auto i = 0; i < n_of_group_elements; i++) {
            if (crypto_scalarmult_ristretto255(
                    exponentiated_group_elements[i],
                    secret_key,
                    group_elements[i]) != 0) {
                return -1;
            }
            // printf("      exp: "); printhex(group_elements[i], group_element_BYTES);
            // printf("           "); printhex(exponentiated_group_elements[i], group_element_BYTES);
        }

        // prepare to prove the exponentiation
        BatchedDLEQProof dleqpfsys(n_of_group_elements + 1); // "+ 1" due to the public key
        assert(2 * (n_of_group_elements + 1) * group_element_BYTES == dleqpfsys.statement_BYTES);
        unsigned char *statement = (unsigned char *) malloc(dleqpfsys.statement_BYTES);

        // prepare statement as (g, a1, ..., an, g^w, a1^w, ..., an^w)
        unsigned char *g = statement;
        unsigned char *g_witness = g + (n_of_group_elements + 1) * group_element_BYTES;

        ristretto255_base_point(g);
        memcpy(g_witness, public_key, group_element_BYTES);
        for (auto i = 0; i < n_of_group_elements; i++) {
            memcpy(g + (i + 1) * group_element_BYTES, group_elements[i], group_element_BYTES);
            memcpy(g_witness + (i + 1) * group_element_BYTES, exponentiated_group_elements[i], group_element_BYTES);
            // printf("     _exp: "); printhex(g + (i + 1) * group_element_BYTES, group_element_BYTES);
            // printf("           "); printhex(g_witness + (i + 1) * group_element_BYTES, group_element_BYTES);
        }

        // printf("       sk: "); printhex((unsigned char *)(secret_key), VEP::secret_key_BYTES);
        // printf("statement: "); printhex(g, dleqpfsys.statement_BYTES/2);
        // printf("           "); printhex(g_witness, dleqpfsys.statement_BYTES/2);

        // prove the exponentiation
        int rv = dleqpfsys.prove(secret_key, statement, proof);
        free(statement);
        return rv;*/
        return 0;
    }

    return -1;
}


template<class ZKProofSystem>
void test_VE(size_t n) {
    // no shuffling
    printf("Testing VE with %zu elements...\n", n);

    VEP<ZKProofSystem> vep(NULL);

    unsigned char secret_key[vep.secret_key_BYTES];
    unsigned char public_key[vep.public_key_BYTES];
    unsigned char key_proof[vep.key_proof_BYTES];
    unsigned char *exponentiation_proof = (unsigned char *) malloc(vep.exponentiation_proof_BYTES);
    bool valid;

    // key generation
    auto started = std::chrono::high_resolution_clock::now();
    if (vep.gen(secret_key, public_key, key_proof) != 0) {
        assert(false);
    }
    auto done = std::chrono::high_resolution_clock::now();
    std::cout << "VE.Gen runtime (µs): " << std::chrono::duration_cast<std::chrono::microseconds>(done - started).
            count() << std::endl;

    // check key generation proof
    DLOG2GenProof dlogpfsys;
    bool validkeys = dlogpfsys.verify(public_key, key_proof);
    assert(validkeys);

    // prepare input group elements
    unsigned char **group_elements = (unsigned char **) malloc(n * sizeof(unsigned char *));
    for (auto i = 0; i < n; i++) {
        group_elements[i] = (unsigned char *) malloc(vep.group_element_BYTES);
        // a_i <- random group element
        crypto_core_ristretto255_random(group_elements[i]);
    }

    // prepare output group elements buffer
    unsigned char **exponentiated_group_elements = (unsigned char **) malloc(n * sizeof(unsigned char *));
    for (auto i = 0; i < n; i++) {
        exponentiated_group_elements[i] = (unsigned char *) malloc(vep.group_element_BYTES);
    }

    // perform VE evaluation
    started = std::chrono::high_resolution_clock::now();
    if (vep.eval(
            secret_key, public_key,
            n, group_elements,
            NULL,
            exponentiated_group_elements,
            exponentiation_proof
        ) != 0) {
        assert(false);
    }
    done = std::chrono::high_resolution_clock::now();
    std::cout << "VE.Eval runtime (µs): " << std::chrono::duration_cast<std::chrono::microseconds>(done - started).
            count() << std::endl;

    // check proof
    started = std::chrono::high_resolution_clock::now();

    valid = vep.check(
        public_key,
        n,
        group_elements,
        exponentiated_group_elements,
        exponentiation_proof
    );
    done = std::chrono::high_resolution_clock::now();
    std::cout << "VE.Check runtime (µs): " << std::chrono::duration_cast<std::chrono::microseconds>(done - started).
            count() << std::endl;
    assert(valid);

    // printf(" --- about to free ---\n");

    // free group elements
    for (auto i = 0; i < n; i++) {
        free(group_elements[i]);
        free(exponentiated_group_elements[i]);
    }
    free(group_elements);
    free(exponentiated_group_elements);
    free(exponentiation_proof);
    printf("about to return\n");
}

template<class SCSigmaProtocol, class ShuffledSCSigmaProtocol>
void test_VEP(size_t t, size_t n) {
    printf("Testing VEP with %zu elements and soundness error 2^{-%zu}...\n", n, t);

    ShuffledSCSigmaProtocol pfsys(n);
    RepeatedSigmaProtocol<ShuffledSCSigmaProtocol> repfsys(t, pfsys);
    FiatShamir<RepeatedSigmaProtocol<ShuffledSCSigmaProtocol> > fsproof(repfsys);

    VEP<FiatShamir<RepeatedSigmaProtocol<ShuffledSCSigmaProtocol> > > vep(&fsproof);

    unsigned char secret_key[vep.secret_key_BYTES];
    unsigned char public_key[vep.public_key_BYTES];
    unsigned char key_proof[vep.key_proof_BYTES];
    unsigned char *exponentiation_proof = (unsigned char *) malloc(vep.exponentiation_proof_BYTES);
    size_t shuffle[n];
    bool valid;

    // key generation
    auto started = std::chrono::high_resolution_clock::now();
    if (vep.gen(secret_key, public_key, key_proof) != 0) {
        assert(false);
    }
    auto done = std::chrono::high_resolution_clock::now();
    std::cout << "VEP.Gen runtime (µs): " << std::chrono::duration_cast<std::chrono::microseconds>(done - started).
            count() << std::endl;

    // check key generation proof
    DLOG2GenProof dlogpfsys;
    bool validkeys = dlogpfsys.verify(public_key, key_proof);
    assert(validkeys);

    // prepare random shuffle
    random_permutation(n, shuffle);

    // prepare input group elements
    unsigned char **group_elements = (unsigned char **) malloc(n * sizeof(unsigned char *));
    for (auto i = 0; i < n; i++) {
        group_elements[i] = (unsigned char *) malloc(vep.group_element_BYTES);
        // a_i <- random group element
        crypto_core_ristretto255_random(group_elements[i]);
    }

    // prepare output group elements buffer
    unsigned char **exponentiated_group_elements = (unsigned char **) malloc(n * sizeof(unsigned char *));
    for (auto i = 0; i < n; i++) {
        exponentiated_group_elements[i] = (unsigned char *) malloc(vep.group_element_BYTES);
    }

    // perform VE evaluation
    started = std::chrono::high_resolution_clock::now();
    if (vep.eval(
            secret_key, public_key,
            n, group_elements,
            shuffle,
            exponentiated_group_elements,
            exponentiation_proof
        ) != 0) {
        assert(false);
    }
    done = std::chrono::high_resolution_clock::now();
    std::cout << "VEP.Eval runtime (ms): " << std::chrono::duration_cast<std::chrono::milliseconds>(done - started).
            count() << std::endl;

    // check proof
    started = std::chrono::high_resolution_clock::now();

    valid = vep.check(
        public_key,
        n,
        group_elements,
        exponentiated_group_elements,
        exponentiation_proof
    );
    done = std::chrono::high_resolution_clock::now();
    std::cout << "VEP.Check runtime (ms): " << std::chrono::duration_cast<std::chrono::milliseconds>(done - started).
            count() << std::endl;
    assert(valid);

    // free group elements
    for (auto i = 0; i < n; i++) {
        free(group_elements[i]);
        free(exponentiated_group_elements[i]);
    }
    free(group_elements);
    free(exponentiated_group_elements);
    free(exponentiation_proof);
}

template void test_VE<BatchedDLEQProof>(size_t n);

#include "shuffle_compatible_dleq.hpp"
#include "shuffled_sigma_protocol.hpp"

template void test_VEP<SCDLEQSigmaProtocol, ShuffledSigmaProtocol<SCDLEQSigmaProtocol> >(size_t t, size_t n);

template class VEP<BatchedDLEQProof>; // c. Added for wasm

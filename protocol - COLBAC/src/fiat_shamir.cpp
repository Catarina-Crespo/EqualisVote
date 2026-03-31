#include <cassert>
#include <cstring>
#include <chrono>
#include <iostream>
#include "utilities.hpp"
#include "base_point.hpp"
#include "random_permutation.hpp"
#include "fiat_shamir.hpp"
#include "repeated_sigma_protocol.hpp"
#include "keccak.hpp"

/*
Fiat Shamir proof system for a Sigma protocol, cf. Definition A.9
*/

// returns 0 if prove succeeded, -1 otherwise
template <class SigmaProtocol>
int FiatShamir<SigmaProtocol>::prove(
    const unsigned char (*witness),
    const unsigned char (*statement),
    unsigned char (*proof)
)
{
    // unpack proof
    unsigned char *salt = proof;
    unsigned char *commitment = salt + crypto_hash_sha256_BYTES;
    unsigned char *response = commitment + inner_pfsys->commitment_BYTES;
    
    // SigmaProtocol sigma_protocol;
    unsigned char state[inner_pfsys->state_BYTES];
    unsigned char challenge[inner_pfsys->challenge_BYTES];

    // generate a uniform SHA256 salt as a way of defining a single-use random oracle
    randombytes_buf(salt, salt_BYTES);

    if (inner_pfsys->prover_commitment(witness, statement, state, commitment) != 0) {
        return -1;
    }

    // challenge generation: challenge <- RO(salt, statement, commitment)
    unsigned char hash_input[salt_BYTES + statement_BYTES + inner_pfsys->commitment_BYTES];
    memcpy(hash_input, salt, salt_BYTES);
    memcpy(hash_input + salt_BYTES, statement, statement_BYTES);
    memcpy(hash_input + salt_BYTES + statement_BYTES, commitment, inner_pfsys->commitment_BYTES);
    FIPS202_SHAKE128(hash_input, salt_BYTES + statement_BYTES + inner_pfsys->commitment_BYTES, challenge, inner_pfsys->challenge_BYTES);

    if (inner_pfsys->prover_response(witness, statement, state, commitment, challenge, response) != 0) {
        return -1;
    }

    return 0;
}

// returns true if the proof is valid, false otherwise
template <class SigmaProtocol>
bool FiatShamir<SigmaProtocol>::verify(
    const unsigned char (*statement),
    const unsigned char (*proof)
)
{
    // SigmaProtocol sigma_protocol;

    // unpack proof
    const unsigned char *salt = proof;
    const unsigned char *commitment = salt + crypto_hash_sha256_BYTES;
    const unsigned char *response = commitment + inner_pfsys->commitment_BYTES;
    
    unsigned char challenge[inner_pfsys->challenge_BYTES];

    // challenge generation: challenge <- RO(salt, statement, commitment)
    unsigned char hash_input[salt_BYTES + statement_BYTES + inner_pfsys->commitment_BYTES];
    memcpy(hash_input, salt, salt_BYTES);
    memcpy(hash_input + salt_BYTES, statement, statement_BYTES);
    memcpy(hash_input + salt_BYTES + statement_BYTES, commitment, inner_pfsys->commitment_BYTES);
    FIPS202_SHAKE128(hash_input, salt_BYTES + statement_BYTES + inner_pfsys->commitment_BYTES, challenge, inner_pfsys->challenge_BYTES);

    bool valid = inner_pfsys->verifier_check(statement, commitment, challenge, response);
    return valid;
}

template<class SigmaProtocol>
void test_FiatShamir_SCDLEQ()
{
    printf("Testing Fiat Shamir of SCDLEQ (very high soundness error)...\n");

    SigmaProtocol inner_pfsys;
    FiatShamir<SigmaProtocol> fsproof(inner_pfsys);

    unsigned char witness[fsproof.witness_BYTES];
    unsigned char statement[fsproof.statement_BYTES];
    unsigned char proof[fsproof.proof_BYTES];
    unsigned char bad_statement[fsproof.statement_BYTES];
    unsigned char bad_proof[fsproof.proof_BYTES];
    bool valid;

    // unpack statement
    unsigned char *g = statement;
    unsigned char *a = g + crypto_core_ristretto255_BYTES;
    unsigned char *g_witness = a + crypto_core_ristretto255_BYTES;
    unsigned char *a_witness = g_witness + crypto_core_ristretto255_BYTES;

    // generate an element of the relation
    crypto_core_ristretto255_scalar_random(witness);
    ristretto255_base_point(g);
    crypto_core_ristretto255_random(a);
    if (crypto_scalarmult_ristretto255(g_witness, witness, g) != 0) {
        assert(false);
    }
    if (crypto_scalarmult_ristretto255(a_witness, witness, a) != 0) {
        assert(false);
    }

    auto started = std::chrono::high_resolution_clock::now();
    if (fsproof.prove(witness, statement, proof) != 0) {
        assert(false);
    }
    auto done = std::chrono::high_resolution_clock::now();
    std::cout << "FSSCDLEQ.Prove runtime (µs): " << std::chrono::duration_cast<std::chrono::microseconds>(done-started).count() << std::endl;

    memcpy(bad_statement, statement, fsproof.statement_BYTES);
    memcpy(bad_proof, proof, fsproof.proof_BYTES);
    bad_statement[0] += 1;
    bad_proof[crypto_hash_sha256_BYTES] += 1;

    started = std::chrono::high_resolution_clock::now();
    valid = fsproof.verify(statement, proof);
    done = std::chrono::high_resolution_clock::now();
    std::cout << "FSSCDLEQ.Verify runtime (µs): " << std::chrono::duration_cast<std::chrono::microseconds>(done-started).count() << std::endl;
    assert(valid);
    // a bad statement/proof may still work due to soundness error
    // valid = fsproof.verify(bad_statement, proof);
    // assert(!valid);
    // valid = fsproof.verify(statement, bad_proof);
    // assert(!valid);
    // valid = fsproof.verify(bad_statement, bad_proof);
    // assert(!valid);
}

#include "shuffle_compatible_dleq.hpp"
template void test_FiatShamir_SCDLEQ<SCDLEQSigmaProtocol>();


template<class SCSigmaProtocol, class ShuffledSCSigmaProtocol>
void test_FiatShamir_RepeatedShuffledDLEQ(size_t t, size_t n)
{
    printf("Testing Fiat Shamir of %zu-Repeated ShuffledDLEQProof with %zu elements...\n", t, n);

    ShuffledSCSigmaProtocol pfsys(n);
    RepeatedSigmaProtocol<ShuffledSCSigmaProtocol> repfsys(t, pfsys);
    FiatShamir<RepeatedSigmaProtocol<ShuffledSCSigmaProtocol>> fsproof(repfsys);

    unsigned char witness[fsproof.witness_BYTES];
    unsigned char statement[fsproof.statement_BYTES];
    unsigned char proof[fsproof.proof_BYTES];
    unsigned char bad_statement[fsproof.statement_BYTES];
    unsigned char bad_proof[fsproof.proof_BYTES];
    bool valid;

    // prepare statement for Shuffled DLEQ
    unsigned char ordered_witness[pfsys.witness_BYTES];

    std::vector<size_t> permutation = random_permutation(n);
    std::copy(permutation.begin(), permutation.end(), (size_t *)(witness + n * SCSigmaProtocol::witness_BYTES));

    // inner protocol
    unsigned char *statements[n];
    unsigned char *statement0[n];
    unsigned char *statement1[n];

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

    for (auto i = 0; i < n; i++)
    {
        // create the witness
        crypto_core_ristretto255_scalar_random(ordered_witness + i * SCSigmaProtocol::witness_BYTES);

        // create the statement
        statements[i] = (unsigned char *)malloc(SCSigmaProtocol::statement_BYTES);
        unsigned char *g = statements[i];
        unsigned char *a = g + SCSigmaProtocol::statement_BYTES/4;
        unsigned char *g_witness = a + SCSigmaProtocol::statement_BYTES/4;
        unsigned char *a_witness = g_witness + SCSigmaProtocol::statement_BYTES/4;
        statement0[i] = g;
        statement1[i] = g_witness;

        ristretto255_base_point(g);
        crypto_core_ristretto255_random(a);
        if (crypto_scalarmult_ristretto255(g_witness, ordered_witness + i * SCSigmaProtocol::witness_BYTES, g) != 0) {
            assert(false);
        }
        if (crypto_scalarmult_ristretto255(a_witness, ordered_witness + i * SCSigmaProtocol::witness_BYTES, a) != 0) {
            assert(false);
        }
    }

    // pack individual exponentiations as single shuffled statement
    for (auto i = 0; i < n; i++)
    {
        size_t shuffled_i = permutation[i];

        // position (gi, ai)
        memcpy(
            statement + i * SCSigmaProtocol::statement_BYTES,
            statement0[i],
            SCSigmaProtocol::statement_BYTES/2
        );

        // let j = pi(i)
        // position (gj^wj, aj^wj)
        memcpy(
            statement + SCSigmaProtocol::statement_BYTES/2 + shuffled_i * SCSigmaProtocol::statement_BYTES,
            statement1[i],
            SCSigmaProtocol::statement_BYTES/2
        );

        // shuffle the witnessess too
        memcpy(
            witness + shuffled_i * SCSigmaProtocol::witness_BYTES,
            ordered_witness + i * SCSigmaProtocol::witness_BYTES,
            SCSigmaProtocol::witness_BYTES
        );
    }

    for (auto i = 0; i < n; i++) {
        free(statements[i]);
    }


    auto started = std::chrono::high_resolution_clock::now();
    if (fsproof.prove(witness, statement, proof) != 0) {
        assert(false);
    }
    auto done = std::chrono::high_resolution_clock::now();
    std::cout << "FSReShDLEQ.Prove runtime (ms): " << std::chrono::duration_cast<std::chrono::milliseconds>(done-started).count() << std::endl;

    memcpy(bad_statement, statement, fsproof.statement_BYTES);
    memcpy(bad_proof, proof, fsproof.proof_BYTES);
    bad_statement[0] += 1;
    bad_proof[crypto_hash_sha256_BYTES] += 1;

    started = std::chrono::high_resolution_clock::now();
    valid = fsproof.verify(statement, proof);
    done = std::chrono::high_resolution_clock::now();
    std::cout << "FSReShDLEQ.Verify runtime (ms): " << std::chrono::duration_cast<std::chrono::milliseconds>(done-started).count() << std::endl;
    assert(valid);
    // a bad statement/proof may still work due to soundness error, I think...
    if (t >= 10)
    {
        valid = fsproof.verify(bad_statement, proof);
        assert(!valid);
        valid = fsproof.verify(statement, bad_proof);
        assert(!valid);
        valid = fsproof.verify(bad_statement, bad_proof);
        assert(!valid);
    }

}


#include "shuffled_sigma_protocol.hpp"
template void test_FiatShamir_RepeatedShuffledDLEQ<SCDLEQSigmaProtocol, ShuffledSigmaProtocol<SCDLEQSigmaProtocol>>(size_t t, size_t n);
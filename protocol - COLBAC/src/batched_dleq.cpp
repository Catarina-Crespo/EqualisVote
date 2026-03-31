#include <cassert>
#include <cstring>
#include <chrono>
#include <iostream>
#include "utilities.hpp"
#include "base_point.hpp"
#include "batched_dleq.hpp"

/*
Proof system for DLEQ, cf. Figure 6
*/

// returns 0 if prove succeeded, -1 otherwise
int BatchedDLEQProof::prove(
    const unsigned char (*witness),
    const unsigned char (*statement),
    unsigned char (*proof)
)
{
    // printf("------ PROVE -----\n");

    // printf("  witness: "); printhex((unsigned char *)(witness), BatchedDLEQProof::witness_BYTES);
    // printf("statement: "); printhex((unsigned char *)(statement), statement_BYTES);

    // unpack statement
    const unsigned char *g = statement;
    const unsigned char *g_witness = g + elements * crypto_core_ristretto255_BYTES;

    // unpack proof
    unsigned char *salt = proof;
    unsigned char *challenge = salt + crypto_hash_sha256_BYTES;
    unsigned char *response = challenge + crypto_core_ristretto255_SCALARBYTES;

    // generate a uniform SHA256 salt as a way of defining a single-use random oracle
    randombytes_buf(salt, crypto_hash_sha256_BYTES);

    // commitment generation: (exponent, commitment) <- (t, ("statement[0]" ^ t, "statement[1]" ^ t, ..., "statement[n-1]"^t))
    unsigned char exponent[crypto_core_ristretto255_SCALARBYTES];
    unsigned char *commitment = (unsigned char *)malloc(commitment_BYTES);
    crypto_core_ristretto255_scalar_random(exponent);

    for (size_t i = 0; i < elements; i++) {
        if (crypto_scalarmult_ristretto255(
            commitment + i * crypto_core_ristretto255_BYTES,
            exponent,
            g + i * crypto_core_ristretto255_BYTES) != 0)
        {
            printf("element %zu is not point on curve\n", i);
            return -1;
        }
    }

    // challenge generation: challenge <- RO(salt, statement, commitment) mod p
    assert(2 * crypto_core_ristretto255_SCALARBYTES >= crypto_hash_sha256_BYTES);
    unsigned char challenge_hash[2 * crypto_core_ristretto255_SCALARBYTES]; // read note on crypto_core_ristretto255_scalar_reduce in libsodium docs
    memset(challenge_hash, 0, 2 * crypto_core_ristretto255_SCALARBYTES);
    crypto_hash_sha256_state state;
    crypto_hash_sha256_init(&state);
    crypto_hash_sha256_update(&state, salt, salt_BYTES);
    crypto_hash_sha256_update(&state, statement, statement_BYTES);
    crypto_hash_sha256_update(&state, commitment, commitment_BYTES);
    crypto_hash_sha256_final(&state, challenge_hash);
    crypto_core_ristretto255_scalar_reduce(challenge, challenge_hash);

    // response generation: response <- exponent + challenge * witness mod p
    // assumes witness is already reduced mod p, which is up to the Prover
    unsigned char challenge_times_witness_mod_p[crypto_core_ristretto255_SCALARBYTES];
    crypto_core_ristretto255_scalar_mul(challenge_times_witness_mod_p, challenge, witness);
    crypto_core_ristretto255_scalar_add(response, exponent, challenge_times_witness_mod_p);

    // printf("    proof: "); printhex((unsigned char *)(proof), crypto_hash_sha256_BYTES);
    // printf("           "); printhex((unsigned char *)(proof+crypto_hash_sha256_BYTES), crypto_core_ristretto255_SCALARBYTES);
    // printf("           "); printhex((unsigned char *)(proof+crypto_hash_sha256_BYTES+crypto_core_ristretto255_SCALARBYTES), crypto_core_ristretto255_SCALARBYTES);

    // printf("------ END PROVE -----\n");

    free(commitment);
    return 0;
}

// returns true if the proof is valid, false otherwise
bool BatchedDLEQProof::verify(
    const unsigned char (*statement),
    const unsigned char (*proof)
)
{
    // printf("------ VERIFY -----\n");
    // unpack statement
    const unsigned char *g = statement;
    const unsigned char *g_witness = g + elements * crypto_core_ristretto255_BYTES;

    // unpack proof
    const unsigned char *salt = proof;
    const unsigned char *challenge = salt + crypto_hash_sha256_BYTES;
    const unsigned char *response = challenge + crypto_core_ristretto255_SCALARBYTES;

    // recreate commitment
    unsigned char a_response[crypto_core_ristretto255_BYTES]; // a^s
    unsigned char negative_challenge[crypto_core_ristretto255_SCALARBYTES]; // -c
    unsigned char a_witness_negative_challenge[crypto_core_ristretto255_BYTES]; // (a^w)^(-c)
    unsigned char *recovered_commitment = (unsigned char *) malloc(commitment_BYTES);  // list of a_i^s * (a_i^w)^(-c) = a_i^(s - w * c)
    crypto_core_ristretto255_scalar_negate(negative_challenge, challenge);
    for (auto i = 0; i < elements; i++) {
        if (crypto_scalarmult_ristretto255(a_response, response, g + i * crypto_core_ristretto255_BYTES) != 0) {
            return false;
        }
        if (crypto_scalarmult_ristretto255(a_witness_negative_challenge, negative_challenge, g_witness + i * crypto_core_ristretto255_BYTES) != 0) {
            return false;
        }
        if (crypto_core_ristretto255_add(recovered_commitment + i * crypto_core_ristretto255_BYTES, a_response, a_witness_negative_challenge) != 0) {
            return false;
        }
    }

    // recreate challenge
    unsigned char recovered_challenge[crypto_core_ristretto255_SCALARBYTES];
    assert(2 * crypto_core_ristretto255_SCALARBYTES >= crypto_hash_sha256_BYTES);
    unsigned char recovered_challenge_hash[2 * crypto_core_ristretto255_SCALARBYTES]; // read note on crypto_core_ristretto255_scalar_reduce in libsodium docs
    memset(recovered_challenge_hash, 0, 2 * crypto_core_ristretto255_SCALARBYTES);
    crypto_hash_sha256_state state;
    crypto_hash_sha256_init(&state);
    crypto_hash_sha256_update(&state, salt, salt_BYTES);
    crypto_hash_sha256_update(&state, statement, statement_BYTES);
    crypto_hash_sha256_update(&state, recovered_commitment, commitment_BYTES);
    crypto_hash_sha256_final(&state, recovered_challenge_hash);
    crypto_core_ristretto255_scalar_reduce(recovered_challenge, recovered_challenge_hash);

    // return true iff challenge and recreated challenge are equal
    bool valid = bool(memcmp(challenge, recovered_challenge, crypto_core_ristretto255_SCALARBYTES) == 0);
    free(recovered_commitment);
    return valid;
}


void test_BatchedDLEQProof(size_t n)
{
    printf("Testing BatchedDLEQProof with %zu elements...\n", n);
    BatchedDLEQProof batched_dleqproof(n);

    unsigned char witness[BatchedDLEQProof::witness_BYTES];
    unsigned char statement[batched_dleqproof.statement_BYTES];
    unsigned char proof[BatchedDLEQProof::proof_BYTES];
    unsigned char bad_statement[batched_dleqproof.statement_BYTES];
    unsigned char bad_proof[BatchedDLEQProof::proof_BYTES];
    bool valid;

    // unpack statement
    unsigned char *g = statement;
    unsigned char *g_witness = g + n * crypto_core_ristretto255_BYTES;

    // generate an element of the relation
    // statement is of the form (g, a_1, ..., a_{n-1}, g^w, a_1^w, ..., a_{n-1}^w)
    crypto_core_ristretto255_scalar_random(witness);
    ristretto255_base_point(g);
    for (auto i = 1; i < n; i++) {
        // a_i <- random group element
        crypto_core_ristretto255_random(g + i * crypto_core_ristretto255_BYTES);
    }
    for (auto i = 0; i < n; i++) {
        // a_i^w
        if (crypto_scalarmult_ristretto255(g_witness + i * crypto_core_ristretto255_BYTES, witness, g + i * crypto_core_ristretto255_BYTES) != 0) {
            assert(false);
        }
    }

    // printf("  witness: "); printhex((unsigned char *)(witness), DLEQProof::witness_BYTES);
    // printf("statement: "); printhex((unsigned char *)(statement), DLEQProof::statement_BYTES);

    auto started = std::chrono::high_resolution_clock::now();
    if (batched_dleqproof.prove(witness, statement, proof) != 0) {
        assert(false);
    }
    auto done = std::chrono::high_resolution_clock::now();
    std::cout << "BatchedDLEQ.Prove runtime (µs): " << std::chrono::duration_cast<std::chrono::microseconds>(done-started).count() << std::endl;

    memcpy(bad_statement, statement, batched_dleqproof.statement_BYTES);
    memcpy(bad_proof, proof, BatchedDLEQProof::proof_BYTES);
    bad_statement[0] += 1;
    bad_proof[crypto_hash_sha256_BYTES] += 1;

    // printf("    proof: "); printhex((unsigned char *)(proof), crypto_hash_sha256_BYTES);
    // printf("           "); printhex((unsigned char *)(proof+crypto_hash_sha256_BYTES), crypto_core_ristretto255_SCALARBYTES);
    // printf("           "); printhex((unsigned char *)(proof+crypto_hash_sha256_BYTES+crypto_core_ristretto255_SCALARBYTES), crypto_core_ristretto255_SCALARBYTES);

    started = std::chrono::high_resolution_clock::now();
    valid = batched_dleqproof.verify(statement, proof);
    done = std::chrono::high_resolution_clock::now();
    std::cout << "BatchedDLEQ.Verify runtime (µs): " << std::chrono::duration_cast<std::chrono::microseconds>(done-started).count() << std::endl;
    assert(valid);
    valid = batched_dleqproof.verify(bad_statement, proof);
    assert(!valid);
    valid = batched_dleqproof.verify(statement, bad_proof);
    assert(!valid);
    valid = batched_dleqproof.verify(bad_statement, bad_proof);
    assert(!valid);
}


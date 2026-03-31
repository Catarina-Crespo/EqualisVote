#include <cassert>
#include <cstring>
#include <chrono>
#include <iostream>
#include "utilities.hpp"
#include "base_point.hpp"

#include "dlog.hpp"

/*
Proof system for DLOG, cf. Figure 6
*/

// returns 0 if prove succeeded, -1 otherwise
int DLOGProof::prove(
    const unsigned char (*witness),
    const unsigned char (*statement),
    unsigned char (*proof)
)
{
    // printf("------ PROVE -----\n");

    // unpack statement
    const unsigned char *g = statement;
    const unsigned char *g_witness = g + crypto_core_ristretto255_BYTES;

    // unpack proof
    unsigned char *salt = proof;
    unsigned char *challenge = salt + crypto_hash_sha256_BYTES;
    unsigned char *response = challenge + crypto_core_ristretto255_SCALARBYTES;

    // generate a uniform SHA256 salt as a way of defining a single-use random oracle
    randombytes_buf(salt, crypto_hash_sha256_BYTES);

    // commitment generation: (exponent, commitment) <- (t, "statement[0]" ^ t)
    unsigned char exponent[crypto_core_ristretto255_SCALARBYTES];
    unsigned char commitment[commitment_BYTES];
    crypto_core_ristretto255_scalar_random(exponent);
    
    if (crypto_scalarmult_ristretto255(commitment, exponent, g) != 0) {
        return -1;
    }

    // challenge generation: challenge <- RO(salt, statement, commitment) mod p
    assert(2 * crypto_core_ristretto255_SCALARBYTES >= crypto_hash_sha256_BYTES);
    unsigned char challenge_hash[2 * crypto_core_ristretto255_SCALARBYTES]; // read note on crypto_core_ristretto255_scalar_reduce
    memset(challenge_hash, 0, 2 * crypto_core_ristretto255_SCALARBYTES);
    crypto_hash_sha256_state state;
    crypto_hash_sha256_init(&state);
    crypto_hash_sha256_update(&state, salt, salt_BYTES);
    crypto_hash_sha256_update(&state, statement, statement_BYTES);
    crypto_hash_sha256_update(&state, commitment, commitment_BYTES);
    crypto_hash_sha256_final(&state, challenge_hash);
    crypto_core_ristretto255_scalar_reduce(challenge, challenge_hash);

    // response generation: response <- exponent + challenge * witness mod p
    unsigned char challenge_times_witness_mod_p[crypto_core_ristretto255_SCALARBYTES];
    crypto_core_ristretto255_scalar_mul(challenge_times_witness_mod_p, challenge, witness);
    crypto_core_ristretto255_scalar_add(response, exponent, challenge_times_witness_mod_p);
    return 0;
}

// returns true if the proof is valid, false otherwise
bool DLOGProof::verify(
    const unsigned char (*statement),
    const unsigned char (*proof)
)
{
    // printf("------ VERIFY -----\n");
    // unpack statement
    const unsigned char *g = statement;
    const unsigned char *g_witness = statement + crypto_core_ristretto255_BYTES;

    // unpack proof
    const unsigned char *salt = proof;
    const unsigned char *challenge = salt + crypto_hash_sha256_BYTES;
    const unsigned char *response = challenge + crypto_core_ristretto255_SCALARBYTES;

    // recreate commitment
    unsigned char g_response[crypto_core_ristretto255_BYTES]; // g^s
    unsigned char negative_challenge[crypto_core_ristretto255_SCALARBYTES]; // -c
    unsigned char g_witness_negative_challenge[crypto_core_ristretto255_BYTES]; // (g^w)^(-c)
    unsigned char recovered_commitment[crypto_core_ristretto255_BYTES]; // g^s * (g^w)^(-c) = g^(s - w * c)
    if (crypto_scalarmult_ristretto255(g_response, response, g) != 0) {
        return false;
    }
    crypto_core_ristretto255_scalar_negate(negative_challenge, challenge);
    if (crypto_scalarmult_ristretto255(g_witness_negative_challenge, negative_challenge, g_witness) != 0) {
        return false;
    }
    if (crypto_core_ristretto255_add(recovered_commitment, g_response, g_witness_negative_challenge) != 0) {
        return false;
    }

    // recreate challenge
    unsigned char recovered_challenge[crypto_core_ristretto255_SCALARBYTES];
    unsigned char recovered_challenge_hash[2 * crypto_hash_sha256_BYTES];
    memset(recovered_challenge_hash, 0, 2 * crypto_hash_sha256_BYTES);
    crypto_hash_sha256_state state;
    crypto_hash_sha256_init(&state);
    crypto_hash_sha256_update(&state, salt, crypto_hash_sha256_BYTES);
    crypto_hash_sha256_update(&state, statement, DLOGProof::statement_BYTES);
    crypto_hash_sha256_update(&state, recovered_commitment, crypto_core_ristretto255_BYTES);
    crypto_hash_sha256_final(&state, recovered_challenge_hash);
    crypto_core_ristretto255_scalar_reduce(recovered_challenge, recovered_challenge_hash);

    // return true iff challenge and recreated challenge are equal
    bool valid = bool(memcmp(challenge, recovered_challenge, crypto_core_ristretto255_SCALARBYTES) == 0);
    return valid;
}


void test_DLOGProof()
{
    printf("Testing DLOGProof...\n");

    unsigned char witness[DLOGProof::witness_BYTES];
    unsigned char statement[DLOGProof::statement_BYTES];
    unsigned char proof[DLOGProof::proof_BYTES];
    unsigned char bad_statement[DLOGProof::statement_BYTES];
    unsigned char bad_proof[DLOGProof::proof_BYTES];
    bool valid;

    // unpack statement
    unsigned char *g = statement;
    unsigned char *g_witness = statement + crypto_core_ristretto255_BYTES;

    // generate an element of the relation
    crypto_core_ristretto255_scalar_random(witness);
    ristretto255_base_point(g);
    if (crypto_scalarmult_ristretto255(g_witness, witness, g) != 0) {
        assert(false);
    }

    // printf("  witness: "); printhex((unsigned char *)(witness), DLOGProof::witness_BYTES);
    // printf("statement: "); printhex((unsigned char *)(statement), DLOGProof::statement_BYTES);

    DLOGProof dlogproof;
    auto started = std::chrono::high_resolution_clock::now();
    if (dlogproof.prove(witness, statement, proof) != 0) {
        assert(false);
    }
    auto done = std::chrono::high_resolution_clock::now();
    std::cout << "DLOG.Prove runtime (µs): " << std::chrono::duration_cast<std::chrono::microseconds>(done-started).count() << std::endl;

    memcpy(bad_statement, statement, DLOGProof::statement_BYTES);
    memcpy(bad_proof, proof, DLOGProof::proof_BYTES);
    bad_statement[0] += 1;
    bad_proof[crypto_hash_sha256_BYTES] += 1;

    // printf("    proof: "); printhex((unsigned char *)(proof), crypto_hash_sha256_BYTES);
    // printf("           "); printhex((unsigned char *)(proof+crypto_hash_sha256_BYTES), crypto_core_ristretto255_SCALARBYTES);
    // printf("           "); printhex((unsigned char *)(proof+crypto_hash_sha256_BYTES+crypto_core_ristretto255_SCALARBYTES), crypto_core_ristretto255_SCALARBYTES);

    started = std::chrono::high_resolution_clock::now();
    valid = dlogproof.verify(statement, proof);
    done = std::chrono::high_resolution_clock::now();
    std::cout << "DLOG.Verify runtime (µs): " << std::chrono::duration_cast<std::chrono::microseconds>(done-started).count() << std::endl;
    assert(valid);
    valid = dlogproof.verify(bad_statement, proof);
    assert(!valid);
    valid = dlogproof.verify(statement, bad_proof);
    assert(!valid);
    valid = dlogproof.verify(bad_statement, bad_proof);
    assert(!valid);
}


#include <cassert>
#include <cstring>
#include <chrono>
#include <iostream>
#include "utilities.hpp"
#include "base_point.hpp"

#include "dlog_to_gen.hpp"
#include "dlog.hpp"

/*
Proof system for DLOG, cf. Figure 6
*/

// returns 0 if prove succeeded, -1 otherwise
int DLOG2GenProof::prove(
    const unsigned char (*witness),
    const unsigned char (*statement),
    unsigned char (*proof)
)
{
    DLOGProof dlogproof;
    unsigned char dlog_statement[DLOGProof::statement_BYTES];
    ristretto255_base_point(dlog_statement);
    memcpy(dlog_statement + crypto_core_ristretto255_BYTES, statement, DLOG2GenProof::statement_BYTES);
    return dlogproof.prove(witness, dlog_statement, proof);
}

// returns true if the proof is valid, false otherwise
bool DLOG2GenProof::verify(
    const unsigned char (*statement),
    const unsigned char (*proof)
)
{
    DLOGProof dlogproof;
    unsigned char dlog_statement[DLOGProof::statement_BYTES];
    ristretto255_base_point(dlog_statement);
    memcpy(dlog_statement + crypto_core_ristretto255_BYTES, statement, DLOG2GenProof::statement_BYTES);
    return dlogproof.verify(dlog_statement, proof);
}


void test_DLOG_to_gen_Proof()
{
    printf("Testing DLOG2GenProof...\n");

    unsigned char g[crypto_core_ristretto255_BYTES];
    unsigned char witness[DLOG2GenProof::witness_BYTES];
    unsigned char statement[DLOG2GenProof::statement_BYTES];
    unsigned char proof[DLOG2GenProof::proof_BYTES];
    unsigned char bad_statement[DLOG2GenProof::statement_BYTES];
    unsigned char bad_proof[DLOG2GenProof::proof_BYTES];
    bool valid;

    // unpack statement
    unsigned char *g_witness = statement;

    // generate an element of the relation
    crypto_core_ristretto255_scalar_random(witness);
    ristretto255_base_point(g);
    if (crypto_scalarmult_ristretto255(g_witness, witness, g) != 0) {
        assert(false);
    }

    // printf("  witness: "); printhex((unsigned char *)(witness), DLOGProof::witness_BYTES);
    // printf("statement: "); printhex((unsigned char *)(statement), DLOGProof::statement_BYTES);

    DLOG2GenProof dlog2genproof;
    auto started = std::chrono::high_resolution_clock::now();
    if (dlog2genproof.prove(witness, statement, proof) != 0) {
        assert(false);
    }
    auto done = std::chrono::high_resolution_clock::now();
    std::cout << "DLOG2Gen.Prove runtime (µs): " << std::chrono::duration_cast<std::chrono::microseconds>(done-started).count() << std::endl;

    memcpy(bad_statement, statement, DLOG2GenProof::statement_BYTES);
    memcpy(bad_proof, proof, DLOG2GenProof::proof_BYTES);
    bad_statement[0] += 1;
    bad_proof[crypto_hash_sha256_BYTES] += 1;

    // printf("    proof: "); printhex((unsigned char *)(proof), crypto_hash_sha256_BYTES);
    // printf("           "); printhex((unsigned char *)(proof+crypto_hash_sha256_BYTES), crypto_core_ristretto255_SCALARBYTES);
    // printf("           "); printhex((unsigned char *)(proof+crypto_hash_sha256_BYTES+crypto_core_ristretto255_SCALARBYTES), crypto_core_ristretto255_SCALARBYTES);

    started = std::chrono::high_resolution_clock::now();
    valid = dlog2genproof.verify(statement, proof);
    done = std::chrono::high_resolution_clock::now();
    std::cout << "DLOG.Verify runtime (µs): " << std::chrono::duration_cast<std::chrono::microseconds>(done-started).count() << std::endl;
    assert(valid);
    valid = dlog2genproof.verify(bad_statement, proof);
    assert(!valid);
    valid = dlog2genproof.verify(statement, bad_proof);
    assert(!valid);
    valid = dlog2genproof.verify(bad_statement, bad_proof);
    assert(!valid);
}


#include <cassert>
#include <cstring>
#include <chrono>
#include <iostream>
#include "utilities.hpp"
#include "base_point.hpp"

#include "trivial_zkproof.hpp"

/*
Proof system for DLOG, cf. Figure 6
*/

// returns 0 if prove succeeded, -1 otherwise
int TrivialProof::prove(
    const unsigned char (*witness),
    const unsigned char (*statement),
    unsigned char (*proof)
)
{
    return 0;
}

// returns true if the proof is valid, false otherwise
bool TrivialProof::verify(
    const unsigned char (*statement),
    const unsigned char (*proof)
)
{
    return true;
}


void test_TrivialProof()
{
    printf("Testing TrivialProof...\n");

    unsigned char *witness = NULL;
    unsigned char *statement = NULL;
    unsigned char *proof = NULL;
    bool valid;

    TrivialProof trivialproof;
    auto started = std::chrono::high_resolution_clock::now();
    if (trivialproof.prove(witness, statement, proof) != 0) {
        assert(false);
    }
    auto done = std::chrono::high_resolution_clock::now();
    std::cout << "TrivialProof.Prove runtime (µs): " << std::chrono::duration_cast<std::chrono::microseconds>(done-started).count() << std::endl;


    started = std::chrono::high_resolution_clock::now();
    valid = trivialproof.verify(statement, proof);
    done = std::chrono::high_resolution_clock::now();
    std::cout << "TrivialProof.Verify runtime (µs): " << std::chrono::duration_cast<std::chrono::microseconds>(done-started).count() << std::endl;
    assert(valid);
}


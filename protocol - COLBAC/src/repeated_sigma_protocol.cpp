#include <cassert>
#include <cstring>
#include <chrono>
#include <iostream>
#include "utilities.hpp"
#include "base_point.hpp"
#include "random_permutation.hpp"
#include "repeated_sigma_protocol.hpp"

/*
Shuffled Sigma protocol from a shuffled-compatible sigma protocol, cf. Fig 1 of 2021/588
Note: starting from relations (w1, (s11, s12)) ... (wn, (sn1, sn2)) and given a permutation pi
we prove a new relation with witness    w_pi(1), ..., w_pi(n)  and statements  (s11, s_pi(1)2) ... (sn1, s_pi(n)2)
meaning after shuffling the right-hand statements, we should shuffle the witnesses too!
*/

// returns 0 if prove succeeded, -1 otherwise
template <class SigmaProtocol>
int RepeatedSigmaProtocol<SigmaProtocol>::prover_commitment(
    const unsigned char (*witness),
    const unsigned char (*statement),
    unsigned char (*state),
    unsigned char (*commitment)
)
{
    int rv = 0;
    for (auto i = 0; i < repetitions; i++)
    {
        unsigned char *inner_state = state + i * inner_pfsys->state_BYTES;
        unsigned char *inner_commitment = commitment + i * inner_pfsys->commitment_BYTES;
        if (inner_pfsys->prover_commitment(witness, statement, inner_state, inner_commitment) != 0) {
            rv = -1;
        }
    }
    return rv;
}

template <class SigmaProtocol>
void RepeatedSigmaProtocol<SigmaProtocol>::verifier_challenge(
    const unsigned char (*statement),
    const unsigned char (*commitment),
    unsigned char (*challenge)
)
{
    for (auto i = 0; i < repetitions; i++)
    {
        const unsigned char *inner_commitment = commitment + i * inner_pfsys->commitment_BYTES;
        unsigned char *inner_challenge = challenge + i * inner_pfsys->challenge_BYTES;
        inner_pfsys->verifier_challenge(statement, inner_commitment, inner_challenge);
    }
}

// returns 0 if prove succeeded, -1 otherwise
template <class SigmaProtocol>
int RepeatedSigmaProtocol<SigmaProtocol>::prover_response(
    const unsigned char (*witness),
    const unsigned char (*statement),
    const unsigned char (*state),
    const unsigned char (*commitment),
    const unsigned char (*challenge),
    unsigned char (*response)
)
{
    int rv = 0;

    for (auto i = 0; i < repetitions; i++)
    {
        const unsigned char *inner_state = state + i * inner_pfsys->state_BYTES;
        const unsigned char *inner_commitment = commitment + i * inner_pfsys->commitment_BYTES;
        const unsigned char *inner_challenge = challenge + i * inner_pfsys->challenge_BYTES;
        unsigned char *inner_response = response + i * inner_pfsys->response_BYTES;
        if (inner_pfsys->prover_response(witness, statement, inner_state, inner_commitment, inner_challenge, inner_response) != 0) {
            rv = -1;
        }
    }

    return rv;
}

// returns true if the proof is valid, false otherwise
template <class SigmaProtocol>
bool RepeatedSigmaProtocol<SigmaProtocol>::verifier_check(
    const unsigned char (*statement),
    const unsigned char (*commitment),
    const unsigned char (*challenge),
    const unsigned char (*response)
)
{

    bool valid = true;

    for (auto i = 0; i < repetitions; i++)
    {
        const unsigned char *inner_commitment = commitment + i * inner_pfsys->commitment_BYTES;
        const unsigned char *inner_challenge = challenge + i * inner_pfsys->challenge_BYTES;
        const unsigned char *inner_response = response + i * inner_pfsys->response_BYTES;

        if (inner_pfsys->verifier_check(statement, inner_commitment, inner_challenge, inner_response) != true) {
            valid = false;
        }
    }

    return valid;

}

template <class SigmaProtocol, class ShuffledSigmaProtocol>
void test_RepeatedSigmaProtocol(size_t t, size_t n)
{
    printf("Testing %zu-Repeated ShuffledDLEQProof with %zu elements...\n", t, n);

    // shuffled protocol witness/statement generation

    ShuffledSigmaProtocol pfsys(n);
    unsigned char ordered_witness[pfsys.witness_BYTES];
    unsigned char witness[pfsys.witness_BYTES];

    std::vector<size_t> permutation = random_permutation(n);
    // std::copy(permutation.begin(), permutation.end(), (size_t *)(ordered_witness + n * SigmaProtocol::witness_BYTES));
    std::copy(permutation.begin(), permutation.end(), (size_t *)(witness + n * SigmaProtocol::witness_BYTES));
    // printf("pi_w: ");
    // for (auto el: permutation) std::cout << el << ' ';
    // printf("\n");

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
        crypto_core_ristretto255_scalar_random(ordered_witness + i * SigmaProtocol::witness_BYTES);

        // create the statement
        statements[i] = (unsigned char *)malloc(SigmaProtocol::statement_BYTES);
        unsigned char *g = statements[i];
        unsigned char *a = g + SigmaProtocol::statement_BYTES/4;
        unsigned char *g_witness = a + SigmaProtocol::statement_BYTES/4;
        unsigned char *a_witness = g_witness + SigmaProtocol::statement_BYTES/4;
        statement0[i] = g;
        statement1[i] = g_witness;

        ristretto255_base_point(g);
        crypto_core_ristretto255_random(a);
        // DEBUG
        // sodium_memzero(g_witness, SigmaProtocol::witness_BYTES);
        // sodium_memzero(a_witness, SigmaProtocol::witness_BYTES);
        // g_witness[0] = 'g';
        // a_witness[0] = 'a';
        // g_witness[0] = (unsigned char)i;
        // a_witness[0] = (unsigned char)i;
        if (crypto_scalarmult_ristretto255(g_witness, ordered_witness + i * SigmaProtocol::witness_BYTES, g) != 0) {
            assert(false);
        }
        if (crypto_scalarmult_ristretto255(a_witness, ordered_witness + i * SigmaProtocol::witness_BYTES, a) != 0) {
            assert(false);
        }
    }

    unsigned char statement[pfsys.statement_BYTES];

    // pack individual exponentiations as single shuffled statement
    for (auto i = 0; i < n; i++)
    {
        size_t shuffled_i = permutation[i];

        // position (gi, ai)
        memcpy(
            statement + i * SigmaProtocol::statement_BYTES,
            statement0[i],
            SigmaProtocol::statement_BYTES/2
        );

        // let j = pi(i)
        // position (gj^wj, aj^wj)
        memcpy(
            statement + SigmaProtocol::statement_BYTES/2 + shuffled_i * SigmaProtocol::statement_BYTES,
            statement1[i],
            SigmaProtocol::statement_BYTES/2
        );

        memcpy(
            witness + shuffled_i * SigmaProtocol::witness_BYTES,
            ordered_witness + i * SigmaProtocol::witness_BYTES,
            SigmaProtocol::witness_BYTES
        );
    }

    // printf("pre-shuffle   witness: "); print_permutation(n, (size_t *)(witness + n * SigmaProtocol::witness_BYTES));
    // for (auto j = 0; j < n; j++) {
    //     printf("         : ");
    //     printhex(const_cast<unsigned char *>(witness + j * SigmaProtocol::witness_BYTES), SigmaProtocol::witness_BYTES);
    // }


    // printf("pre-shuffle statements: \n"); 
    // for (auto j = 0; j < n; j++) {
    //     printf("         : ");
    //     printhex(statements[j], SigmaProtocol::statement_BYTES);
    // }

    // printf("witness: "); print_permutation(n, (size_t *)(witness + n * SigmaProtocol::witness_BYTES));
    // for (auto j = 0; j < n; j++) {
    //     printf("         : ");
    //     printhex(const_cast<unsigned char *>(shuffled_witness + j * SigmaProtocol::witness_BYTES), SigmaProtocol::witness_BYTES);
    // }

    // printf("statement: \n"); 
    // for (auto j = 0; j < n; j++) {
    //     printf("         : ");
    //     printhex(const_cast<unsigned char *>(statement + j * SigmaProtocol::statement_BYTES), SigmaProtocol::statement_BYTES);
    // }

    // prepare the repeated protocol run

    RepeatedSigmaProtocol<ShuffledSigmaProtocol> repfsys(t, pfsys);

    unsigned char state[repfsys.state_BYTES];
    unsigned char commitment[repfsys.commitment_BYTES];
    unsigned char challenge[repfsys.challenge_BYTES];
    unsigned char response[repfsys.response_BYTES];
    unsigned char bad_response[repfsys.response_BYTES];
    bool valid;

    if (repfsys.prover_commitment(witness, statement, state, commitment) != 0) {
        assert(false);
    }

    // printf("    state: "); print_permutation(n, (size_t *)(state + n * SigmaProtocol::state_BYTES));
    // for (auto j = 0; j < n; j++) {
    //     printf("         : ");
    //     printhex(const_cast<unsigned char *>(state + j * SigmaProtocol::state_BYTES), SigmaProtocol::state_BYTES);
    // }

    // printf("commitment: \n"); 
    // for (auto j = 0; j < n; j++) {
    //     printf("         : ");
    //     printhex(const_cast<unsigned char *>(commitment + j * SigmaProtocol::commitment_BYTES), SigmaProtocol::commitment_BYTES);
    // }

    repfsys.verifier_challenge(statement, commitment, challenge);

    // printf("challenge: "); printhex(challenge, SigmaProtocol::challenge_BYTES);

    if (repfsys.prover_response(witness, statement, state, commitment, challenge, response) != 0) {
        assert(false);
    }

    // printf(" response: "); print_permutation(n, (size_t *)(response + n * SigmaProtocol::response_BYTES));
    // for (auto j = 0; j < n; j++) {
    //     printf("         : ");
    //     printhex(const_cast<unsigned char *>(response + j * SigmaProtocol::response_BYTES), SigmaProtocol::response_BYTES);
    // }

    valid = repfsys.verifier_check(statement, commitment, challenge, response);
    assert(valid);
    memcpy(bad_response, response, repfsys.response_BYTES);
    bad_response[0] += 1;
    valid = repfsys.verifier_check(statement, commitment, challenge, bad_response);
    assert(!valid);

    for (auto i = 0; i < n; i++) {
        free(statements[i]);
    }
}

#include "shuffled_sigma_protocol.hpp"
#include "shuffle_compatible_dleq.hpp"

template void test_RepeatedSigmaProtocol<SCDLEQSigmaProtocol, ShuffledSigmaProtocol<SCDLEQSigmaProtocol>>(size_t t, size_t n);

// Added for emscript
template int RepeatedSigmaProtocol<ShuffledSigmaProtocol<SCDLEQSigmaProtocol>>::prover_commitment(const unsigned char (*witness),
    const unsigned char (*statement),
    unsigned char (*state),
    unsigned char (*commitment));
template void RepeatedSigmaProtocol<ShuffledSigmaProtocol<SCDLEQSigmaProtocol>>::verifier_challenge(const unsigned char (*statement),
    const unsigned char (*commitment),
    unsigned char (*challenge));
template int RepeatedSigmaProtocol<ShuffledSigmaProtocol<SCDLEQSigmaProtocol>>::prover_response(const unsigned char (*witness),
    const unsigned char (*statement),
    const unsigned char (*state),
    const unsigned char (*commitment),
    const unsigned char (*challenge),
    unsigned char (*response));
template bool RepeatedSigmaProtocol<ShuffledSigmaProtocol<SCDLEQSigmaProtocol>>::verifier_check(const unsigned char (*statement),
    const unsigned char (*commitment),
    const unsigned char (*challenge),
    const unsigned char (*response));

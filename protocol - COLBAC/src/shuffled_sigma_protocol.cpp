#include <cassert>
#include <cstring>
#include <chrono>
#include <iostream>
#include "utilities.hpp"
#include "base_point.hpp"
#include "shuffled_sigma_protocol.hpp"
#include "random_permutation.hpp"

/*
Shuffled Sigma protocol from a shuffled-compatible sigma protocol, cf. Fig 1 of 2021/588
Note: starting from relations (w1, (s11, s12)) ... (wn, (sn1, sn2)) and given a permutation pi
we prove a new relation with witness    w_pi(1), ..., w_pi(n)  and statements  (s11, s_pi(1)2) ... (sn1, s_pi(n)2)
meaning after shuffling the right-hand statements, we should shuffle the witnesses too!
*/

// returns 0 if prove succeeded, -1 otherwise
template <class SCSigmaProtocol>
int ShuffledSigmaProtocol<SCSigmaProtocol>::prover_commitment(
    const unsigned char (*witness),
    const unsigned char (*statement),
    unsigned char (*state),
    unsigned char (*commitment)
)
{
    std::vector<size_t> perm_a = random_permutation(elements);
    std::copy(
        perm_a.begin(),
        perm_a.end(),
        (size_t *)(state + elements * SCSigmaProtocol::state_BYTES)
    );

    // printf("---- P 1 ----\n");
    // unpack witness
    size_t *perm_w = (size_t *)(witness + elements * SCSigmaProtocol::witness_BYTES);
    size_t inv_perm_w[elements];
    inverse_permutation(elements, perm_w, inv_perm_w);
    // printf("pi_w: "); print_permutation(elements, perm_w);
    // printf("pi_a: "); print_permutation(perm_a);

    // unpack statement
    unsigned char *statement0[elements];
    unsigned char *statement1[elements];
    for (auto j = 0; j < elements; j++)
    {
        statement0[j] = const_cast<unsigned char *>(statement + j * SCSigmaProtocol::statement_BYTES);
        statement1[j] = statement0[j] + SCSigmaProtocol::statement_BYTES/2;
    }

    // generate commitments using inner protocol
    SCSigmaProtocol sub_pfsys;

    unsigned char ordered_sub_commitments[elements * SCSigmaProtocol::commitment_BYTES];
    for (auto j = 0; j < elements; j++)
    {
        size_t shuffled_j = inv_perm_w[j];
        // std::cout << "P1: j " << j << " -> pi(j) " << shuffled_j << std::endl;
    
        // prepare witness and statement for inner shuffle-compatible sigma protocol
        unsigned char *sub_witness = const_cast<unsigned char *>(witness + j * SCSigmaProtocol::witness_BYTES);
        unsigned char sub_statement[SCSigmaProtocol::statement_BYTES];
        memcpy(
            sub_statement,
            statement0[shuffled_j],
            SCSigmaProtocol::statement_BYTES/2
        );
        memcpy(
            sub_statement + SCSigmaProtocol::statement_BYTES/2,
            statement1[j],
            SCSigmaProtocol::statement_BYTES/2
        );

        // printf("subst pi_w(%ld): ", j);
        // printhex(sub_statement, SCSigmaProtocol::statement_BYTES);

        // generate commitment of inner protocol
        unsigned char *sub_commitment = ordered_sub_commitments + j * SCSigmaProtocol::commitment_BYTES;
        unsigned char *sub_state = state + j * SCSigmaProtocol::state_BYTES;

        if (sub_pfsys.prover_commitment(
            sub_witness,
            sub_statement,
            sub_state,
            sub_commitment
        ) != 0) {
            return -1;
        };
        // printf("commt pi_w(%ld): ", j);
        // printhex(sub_commitment, SCSigmaProtocol::commitment_BYTES);
    }
    // reorder sub_commitments into output commitment
    for (auto j = 0; j < elements; j++)
    {
        memcpy(
            commitment + perm_a[j] * SCSigmaProtocol::commitment_BYTES,
            ordered_sub_commitments + j * SCSigmaProtocol::commitment_BYTES,
            SCSigmaProtocol::commitment_BYTES
        );
    }

    return 0;
}

template <class SCSigmaProtocol>
void ShuffledSigmaProtocol<SCSigmaProtocol>::verifier_challenge(
    const unsigned char (*statement),
    const unsigned char (*commitment),
    unsigned char (*challenge)
)
{
    // printf("---- V1 ----\n");
    uint32_t rand_int = randombytes_random();
    challenge[0] =  *((unsigned char *)(&rand_int));
    // challenge[0] = 0x00; // DEBUG
}

// returns 0 if prove succeeded, -1 otherwise
template <class SCSigmaProtocol>
int ShuffledSigmaProtocol<SCSigmaProtocol>::prover_response(
    const unsigned char (*witness),
    const unsigned char (*statement),
    const unsigned char (*state),
    const unsigned char (*commitment),
    const unsigned char (*challenge),
    unsigned char (*response)
)
{
    // printf("---- P2 ----\n");

    size_t *perm_w = (size_t *)(witness + elements * SCSigmaProtocol::witness_BYTES);
    // printf("       pi_w: "); print_permutation(elements, perm_w);
    size_t *perm_a = (size_t *)(state + elements * SCSigmaProtocol::state_BYTES);
    // printf("       pi_a: "); print_permutation(elements, perm_a);
    size_t inv_perm_a[elements];
    inverse_permutation(elements, perm_a, inv_perm_a);
    // printf("   inv_pi_a: "); print_permutation(elements, inv_perm_a);
    size_t perm_a_comp_perm_w[elements];
    compose_permutations(elements, perm_a, perm_w, perm_a_comp_perm_w);
    size_t perm_w_comp_perm_a[elements];
    compose_permutations(elements, perm_w, perm_a, perm_w_comp_perm_a);
    size_t inv_perm_w[elements];
    inverse_permutation(elements, perm_w, inv_perm_w);
    // printf("pi_a o pi_w: "); print_permutation(elements, perm_a_comp_perm_w);
    // printf("pi_w o pi_a: "); print_permutation(elements, perm_w_comp_perm_a);
    // size_t perm_a_comp_inv_perm_a[elements];
    // size_t inv_perm_a_comp_perm_a[elements];
    // compose_permutations(elements, perm_a, inv_perm_a, perm_a_comp_inv_perm_a);
    // compose_permutations(elements, inv_perm_a, perm_a, inv_perm_a_comp_perm_a);
    // printf("pi_a o  inv: "); print_permutation(elements, perm_a_comp_inv_perm_a);
    // printf("inv  o pi_a: "); print_permutation(elements, inv_perm_a_comp_perm_a);

    size_t perm_z[elements];
    unsigned char challenge_bit = challenge[0] & 0x01;
    if (challenge_bit == 0x00) {
        // assert(false);
        memcpy(perm_z, perm_a_comp_perm_w, elements * sizeof(size_t));
        // memcpy(perm_z, perm_w_comp_perm_a, elements * sizeof(size_t));
    } else {
        memcpy(perm_z, perm_a, elements * sizeof(size_t));
    }
    // printf("       pi_z: "); print_permutation(elements, perm_z);

    // unpack statement
    unsigned char *statement0[elements];
    unsigned char *statement1[elements];
    for (auto j = 0; j < elements; j++)
    {
        statement0[j] = const_cast<unsigned char *>(statement + j * SCSigmaProtocol::statement_BYTES);
        statement1[j] = statement0[j] + SCSigmaProtocol::statement_BYTES/2;
    }

    SCSigmaProtocol sub_pfsys;
    unsigned char ordered_sub_response[elements * SCSigmaProtocol::response_BYTES];
    for (auto j = 0; j < elements; j++)
    {
        size_t shuffled_j = inv_perm_w[j];
        // std::cout << "P1: j " << j << " -> pi(j) " << shuffled_j << std::endl;
    
        // prepare witness and statement for inner shuffle-compatible sigma protocol
        unsigned char *sub_witness = const_cast<unsigned char *>(witness + j * SCSigmaProtocol::witness_BYTES);
        unsigned char sub_statement[SCSigmaProtocol::statement_BYTES];
        memcpy(
            sub_statement,
            statement0[shuffled_j],
            SCSigmaProtocol::statement_BYTES/2
        );
        memcpy(
            sub_statement + SCSigmaProtocol::statement_BYTES/2,
            statement1[j],
            SCSigmaProtocol::statement_BYTES/2
        );
        unsigned char *sub_state = const_cast<unsigned char *>(state + j * SCSigmaProtocol::state_BYTES); //????
        unsigned char *sub_commitment = const_cast<unsigned char *>(commitment + perm_a[j] * SCSigmaProtocol::commitment_BYTES);
        unsigned char *sub_response = ordered_sub_response + j * SCSigmaProtocol::response_BYTES;

        if (sub_pfsys.prover_response(
            sub_witness,
            sub_statement,
            sub_state,
            sub_commitment,
            challenge,
            sub_response
        ) != 0) {
            return -1;
        };

        // printf("witns ?(%ld): ", j);
        // printhex(sub_witness, SCSigmaProtocol::witness_BYTES);

        // printf("subst ?(%ld): ", j);
        // printhex(sub_statement, SCSigmaProtocol::statement_BYTES);

        // printf("state ?(%ld): ", j);
        // printhex(sub_state, SCSigmaProtocol::state_BYTES);

        // printf("commt ?(%ld): ", j);
        // printhex(sub_commitment, SCSigmaProtocol::commitment_BYTES);

        // printf("respn ?(%ld): ", j);
        // printhex(sub_response, SCSigmaProtocol::response_BYTES);

        // printf("\n");
    }
    // reorder sub_responses into output response
    for (auto j = 0; j < elements; j++)
    {
        memcpy(
            response + perm_a[j] * SCSigmaProtocol::response_BYTES,
            ordered_sub_response + j * SCSigmaProtocol::response_BYTES,
            SCSigmaProtocol::response_BYTES
        );
    }
    // copy perm_z into response too
    memcpy(response + elements * SCSigmaProtocol::response_BYTES, perm_z, elements * sizeof(size_t));

    return 0;
}

// returns true if the proof is valid, false otherwise
template <class SCSigmaProtocol>
bool ShuffledSigmaProtocol<SCSigmaProtocol>::verifier_check(
    const unsigned char (*statement),
    const unsigned char (*commitment),
    const unsigned char (*challenge),
    const unsigned char (*response)
)
{
    // printf("---- V2 ----\n");

    // unpack statement
    unsigned char **statement_tuple[2];
    unsigned char *statement0[elements];
    unsigned char *statement1[elements];
    for (auto j = 0; j < elements; j++)
    {
        statement0[j] = const_cast<unsigned char *>(statement + j * SCSigmaProtocol::statement_BYTES);
        statement1[j] = statement0[j] + SCSigmaProtocol::statement_BYTES/2;
    }
    statement_tuple[0x00] = statement0;
    statement_tuple[0x01] = statement1;

    size_t *perm_z = (size_t *)(response + elements * SCSigmaProtocol::response_BYTES);
    // printf("       pi_z: "); print_permutation(elements, perm_z);
    size_t inv_perm_z[elements];
    inverse_permutation(elements, perm_z, inv_perm_z);

    bool valid = true;
    unsigned char challenge_bit = challenge[0] & 0x01;
    SCSigmaProtocol sub_pfsys;
    for (auto j = 0; j < elements; j++)
    {
        unsigned char *sub_commitment = const_cast<unsigned char *>(commitment + j * SCSigmaProtocol::commitment_BYTES);
        unsigned char *sub_response = const_cast<unsigned char *>(response + j * SCSigmaProtocol::response_BYTES);
        unsigned char sub_statement[SCSigmaProtocol::statement_BYTES];
        // here we copy the same half of the statement being checked on both locations
        // being shuffle-compatible, this should not matter for the verification step
        memcpy(
            sub_statement,
            statement_tuple[challenge_bit][inv_perm_z[j]],
            SCSigmaProtocol::statement_BYTES/2
        );
        memcpy(
            sub_statement + SCSigmaProtocol::statement_BYTES/2,
            statement_tuple[challenge_bit][inv_perm_z[j]],
            SCSigmaProtocol::statement_BYTES/2
        );

        // printf("subst pi_z(%ld): ", j);
        // printhex(sub_statement, SCSigmaProtocol::statement_BYTES);

        // printf("commt pi_z(%ld): ", j);
        // printhex(sub_commitment, SCSigmaProtocol::commitment_BYTES);

        // printf("respn pi_z(%ld): ", j);
        // printhex(sub_response, SCSigmaProtocol::response_BYTES);

        if (sub_pfsys.verifier_check(
            sub_statement,
            sub_commitment,
            challenge,
            sub_response
        ) == false)
        {
            // printf("check fails on j = %ld\n", j);
            valid = false;
        } else {
            // printf("check passes on j = %ld\n", j);
        }
    }

    return valid;
}

template <class SigmaProtocol>
void test_ShuffledSigmaProtocol_high_error(size_t n)
{
    printf("Testing ShuffledDLEQProof with %zu elements...\n", n);

    // shuffled protocol
    ShuffledSigmaProtocol<SigmaProtocol> pfsys(n);
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
    unsigned char state[pfsys.state_BYTES];
    unsigned char commitment[pfsys.commitment_BYTES];
    unsigned char challenge[pfsys.challenge_BYTES];
    unsigned char response[pfsys.response_BYTES];
    unsigned char bad_response[pfsys.response_BYTES];
    bool valid;

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

    if (pfsys.prover_commitment(witness, statement, state, commitment) != 0) {
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

    pfsys.verifier_challenge(statement, commitment, challenge);

    // printf("challenge: "); printhex(challenge, SigmaProtocol::challenge_BYTES);

    if (pfsys.prover_response(witness, statement, state, commitment, challenge, response) != 0) {
        assert(false);
    }

    // printf(" response: "); print_permutation(n, (size_t *)(response + n * SigmaProtocol::response_BYTES));
    // for (auto j = 0; j < n; j++) {
    //     printf("         : ");
    //     printhex(const_cast<unsigned char *>(response + j * SigmaProtocol::response_BYTES), SigmaProtocol::response_BYTES);
    // }

    valid = pfsys.verifier_check(statement, commitment, challenge, response);
    assert(valid);
    memcpy(bad_response, response, pfsys.response_BYTES);
    bad_response[0] += 1;
    valid = pfsys.verifier_check(statement, commitment, challenge, bad_response);
    assert(!valid);

    for (auto i = 0; i < n; i++) {
        free(statements[i]);
    }
}


#include "shuffle_compatible_dleq.hpp"
template void test_ShuffledSigmaProtocol_high_error<SCDLEQSigmaProtocol>(size_t n);

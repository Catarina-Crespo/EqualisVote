#include <cassert>
#include <cstring>
#include <chrono>
#include <iostream>
#include "utilities.hpp"
#include "base_point.hpp"
#include "shuffle_compatible_dleq.hpp"

/*
Shuffle compatible Sigma protocol for DLEQ, cf. Def A.12
*/

// returns 0 if prove succeeded, -1 otherwise
int SCDLEQSigmaProtocol::prover_commitment(
    const unsigned char (*witness),
    const unsigned char (*statement),
    unsigned char (*state),
    unsigned char (*commitment)
)
{
    // unpack statement
    const unsigned char *g = statement;
    const unsigned char *a = g + crypto_core_ristretto255_BYTES;
    const unsigned char *g_witness = a + crypto_core_ristretto255_BYTES;
    const unsigned char *a_witness = g_witness + crypto_core_ristretto255_BYTES;

    // commitment generation: (state, commitment) <- (t, ("statement[0]" ^ t, "statement[1]" ^ t))
    unsigned char *commitment_0 = commitment;
    unsigned char *commitment_1 = commitment + crypto_core_ristretto255_BYTES;
    crypto_core_ristretto255_scalar_random(state);
    
    if (crypto_scalarmult_ristretto255(commitment_0, state, g) != 0) {
        return -1;
    }
    if (crypto_scalarmult_ristretto255(commitment_1, state, a) != 0) {
        return -1;
    }

    return 0;
}

void SCDLEQSigmaProtocol::verifier_challenge(
    const unsigned char (*statement),
    const unsigned char (*commitment),
    unsigned char (*challenge)
)
{
    uint32_t rand_int = randombytes_random();
    challenge[0] =  *((unsigned char *)(&rand_int));
}

// returns 0 if prove succeeded, -1 otherwise
int SCDLEQSigmaProtocol::prover_response(
    const unsigned char (*witness),
    const unsigned char (*statement),
    const unsigned char (*state),
    const unsigned char (*commitment),
    const unsigned char (*challenge),
    unsigned char (*response)
)
{
    unsigned char challenge_bit = challenge[0] & 0x01;
    if (challenge_bit == 0x00) {
        assert(response_BYTES == state_BYTES);
        memcpy(response, state, state_BYTES);
    } else {
        unsigned char inverse_witness[crypto_core_ristretto255_BYTES];
        if (crypto_core_ristretto255_scalar_invert(inverse_witness, witness) != 0) {
            return -1;
        }
        crypto_core_ristretto255_scalar_mul(response, state, inverse_witness);
    }
    return 0;
}

// returns true if the proof is valid, false otherwise
bool SCDLEQSigmaProtocol::verifier_check(
    const unsigned char (*statement),
    const unsigned char (*commitment),
    const unsigned char (*challenge),
    const unsigned char (*response)
)
{
    // unpack statement  x_0 = (g, a), x_1 = (g^w, a^w)
    const unsigned char *g = statement;
    const unsigned char *a = g + crypto_core_ristretto255_BYTES;
    const unsigned char *g_witness = a + crypto_core_ristretto255_BYTES;
    const unsigned char *a_witness = g_witness + crypto_core_ristretto255_BYTES;

    // unpack commitment
    const unsigned char *commitment_0 = commitment;
    const unsigned char *commitment_1 = commitment + crypto_core_ristretto255_BYTES;

    // check commitment_0 != group_identity and commitment_1 != group_identity
    unsigned char identity[crypto_core_ristretto255_BYTES];
    ristretto255_identity_point(identity);

    if (memcmp(commitment_0, identity, crypto_core_ristretto255_BYTES) == 0) {
        return false;
    }
    if (memcmp(commitment_1, identity, crypto_core_ristretto255_BYTES) == 0) {
        return false;
    }

    // check statement[e]^response == commitment
    const unsigned char *statement_e_0;
    const unsigned char *statement_e_1;
    unsigned char challenge_bit = challenge[0] & 0x01;

    if (challenge_bit == 0x00) {
        statement_e_0 = g;
        statement_e_1 = a;
    } else {
        statement_e_0 = g_witness;
        statement_e_1 = a_witness;
    }
    unsigned char statement_e_0_response[crypto_core_ristretto255_BYTES];
    unsigned char statement_e_1_response[crypto_core_ristretto255_BYTES];
    if (crypto_scalarmult_ristretto255(statement_e_0_response, response, statement_e_0) != 0) {
        return false;
    }
    if (crypto_scalarmult_ristretto255(statement_e_1_response, response, statement_e_1) != 0) {
        return false;
    }

    if (memcmp(commitment_0, statement_e_0_response, crypto_core_ristretto255_BYTES) != 0) {
        return false;
    }
    if (memcmp(commitment_1, statement_e_1_response, crypto_core_ristretto255_BYTES) != 0) {
        return false;
    }

    return true;
}


void test_shuffle_compatible_DLEQ_SigmaProtocol()
{
    printf("Testing shuffle compatible DLEQ Sigma protocol...\n");

    unsigned char witness[SCDLEQSigmaProtocol::witness_BYTES];
    unsigned char statement[SCDLEQSigmaProtocol::statement_BYTES];
    unsigned char state[SCDLEQSigmaProtocol::state_BYTES];
    unsigned char commitment[SCDLEQSigmaProtocol::commitment_BYTES];
    unsigned char challenge[SCDLEQSigmaProtocol::challenge_BYTES];
    unsigned char response[SCDLEQSigmaProtocol::response_BYTES];
    unsigned char bad_response[SCDLEQSigmaProtocol::response_BYTES];
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

    SCDLEQSigmaProtocol sigma_protocol;
    if (sigma_protocol.prover_commitment(witness, statement, state, commitment) != 0) {
        assert(false);
    }

    sigma_protocol.verifier_challenge(statement, commitment, challenge);
    
    if (sigma_protocol.prover_response(witness, statement, state, commitment, challenge, response) != 0) {
        assert(false);
    }

    valid = sigma_protocol.verifier_check(statement, commitment, challenge, response);
    assert(valid);
    memcpy(bad_response, response, SCDLEQSigmaProtocol::response_BYTES);
    bad_response[0] += 1;
    valid = sigma_protocol.verifier_check(statement, commitment, challenge, bad_response);
    assert(!valid);
}


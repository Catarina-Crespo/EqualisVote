#pragma once

#ifdef __cplusplus
extern "C"{
#endif
#include <sodium.h>
#ifdef __cplusplus
}
#endif

#include <chrono>

struct protocol_measurements {
    // runtime
    int64_t ve_prove = 0.;
    int64_t ve_verify = 0.;
    int64_t user_vep_prove = 0.;
    int64_t user_vep_verify = 0.;
    int64_t server_vep_prove = 0.;
    int64_t server_vep_verify = 0.;
    int64_t intersection = 0.;
    int64_t pre_protocol = 0.;
    int64_t full_protocol = 0.;

    // bandwidth
    size_t ve_statement_BYTES = 0;
    size_t ve_proof_BYTES = 0;
    size_t user_vep_input_BYTES = 0;
    size_t user_vep_statement_BYTES = 0;
    size_t user_vep_proof_BYTES = 0;
    size_t server_vep_input_BYTES = 0;
    size_t server_vep_statement_BYTES = 0;
    size_t server_vep_proof_BYTES = 0;
    size_t ballots_BYTES = 0;
    size_t full_protocol_BYTES = 0;
};

protocol_measurements protocol_run(
    size_t initial_group_size,
    size_t n_voting_users,
    int min_vote = 0,
    int max_vote = 10
);

void print_measurement(protocol_measurements measurements);

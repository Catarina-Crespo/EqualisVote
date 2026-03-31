#ifdef __cplusplus
extern "C"{
#endif
#include <sodium.h>
#ifdef __cplusplus
}
#endif

#include <cstdio>
#include <iostream>
#include "utilities.hpp"
#include "dlog.hpp"
#include "dlog_to_gen.hpp"
#include "dleq.hpp"
#include "batched_dleq.hpp"
#include "shuffle_compatible_dleq.hpp"
#include "fiat_shamir.hpp"
#include "trivial_zkproof.hpp"
#include "verifiable_exponentiation.hpp"
#include "shuffled_sigma_protocol.hpp"
#include "repeated_sigma_protocol.hpp"
#include "protocol_run.hpp"

void benchmarks(size_t group_size, size_t votes, int max_vote = 10);
void test_zk_proofs();

int main(void)
{
    if (sodium_init() < 0) {
        /* panic! the library couldn't be initialized; it is not safe to use */
        printf("ERROR: libsodium not found. exiting.\n");
        exit(1);
    }
    printf("All is good with libsodium version %s.\n", sodium_version_string());

    std::cerr << "{" << std::endl;
    // benchmarks(5, 4); std::cerr << std::endl; // debug
    benchmarks(50, 40); std::cerr << "," << std::endl;
    benchmarks(100, 40); std::cerr << "," << std::endl;
    benchmarks(200, 40); std::cerr << "," << std::endl;
    benchmarks(200, 80); std::cerr << std::endl;
    std::cerr << "}" << std::endl;

    return 0;
}

void benchmarks(size_t group_size, size_t votes, int max_vote)
{
    protocol_measurements measurements;
    size_t tries = 10;
    
    std::cerr << "\"" << group_size << "_" << votes << "_" << max_vote << "\": [" << std::endl;
    for (auto i = 0; i < tries; i++)
    {
        measurements = protocol_run(group_size, votes, 0, max_vote);
        print_measurement(measurements);
        if (i < tries-1) {
            std::cerr << "," << std::endl;
        }
    } 
    std::cerr << "]" << std::endl;
}

void test_zk_proofs()
{
    test_TrivialProof();

    test_DLOGProof(); // (g, g^w)
    test_DLOG_to_gen_Proof(); // g^w  (assumes g is the ristretto255 generator)
    test_DLEQProof(); // (g, a, g^w, a^w)

    // DLEQ with "n elements" means the statement being (g, a_1, ..., a_{n-1}, g^w, a_1^w, ..., a_{n-1}^w)
    test_BatchedDLEQProof(1); // this is essentially DLOGProof
    test_BatchedDLEQProof(2);
    test_BatchedDLEQProof(3);
    test_BatchedDLEQProof(10);

    test_shuffle_compatible_DLEQ_SigmaProtocol(); // (g, a, g^w, a^w)
    test_FiatShamir_SCDLEQ<SCDLEQSigmaProtocol>(); // high soundness error

    test_VE<BatchedDLEQProof>(1); // this is essentially DLEQ
    test_VE<BatchedDLEQProof>(2);
    test_VE<BatchedDLEQProof>(3);
    test_VE<BatchedDLEQProof>(10);
    test_ShuffledSigmaProtocol_high_error<SCDLEQSigmaProtocol>(1);
    test_ShuffledSigmaProtocol_high_error<SCDLEQSigmaProtocol>(2);
    test_ShuffledSigmaProtocol_high_error<SCDLEQSigmaProtocol>(3);
    test_ShuffledSigmaProtocol_high_error<SCDLEQSigmaProtocol>(10);

    test_RepeatedSigmaProtocol<SCDLEQSigmaProtocol, ShuffledSigmaProtocol<SCDLEQSigmaProtocol>>(1, 10);
    test_RepeatedSigmaProtocol<SCDLEQSigmaProtocol, ShuffledSigmaProtocol<SCDLEQSigmaProtocol>>(32, 10);
    test_RepeatedSigmaProtocol<SCDLEQSigmaProtocol, ShuffledSigmaProtocol<SCDLEQSigmaProtocol>>(64, 10);
    test_RepeatedSigmaProtocol<SCDLEQSigmaProtocol, ShuffledSigmaProtocol<SCDLEQSigmaProtocol>>(128, 10);

    test_FiatShamir_RepeatedShuffledDLEQ<SCDLEQSigmaProtocol, ShuffledSigmaProtocol<SCDLEQSigmaProtocol>>(1, 10);
    test_FiatShamir_RepeatedShuffledDLEQ<SCDLEQSigmaProtocol, ShuffledSigmaProtocol<SCDLEQSigmaProtocol>>(32, 10);
    test_FiatShamir_RepeatedShuffledDLEQ<SCDLEQSigmaProtocol, ShuffledSigmaProtocol<SCDLEQSigmaProtocol>>(64, 10);
    test_FiatShamir_RepeatedShuffledDLEQ<SCDLEQSigmaProtocol, ShuffledSigmaProtocol<SCDLEQSigmaProtocol>>(128, 10);
    test_FiatShamir_RepeatedShuffledDLEQ<SCDLEQSigmaProtocol, ShuffledSigmaProtocol<SCDLEQSigmaProtocol>>(128, 100);
    test_FiatShamir_RepeatedShuffledDLEQ<SCDLEQSigmaProtocol, ShuffledSigmaProtocol<SCDLEQSigmaProtocol>>(128, 200);

    test_FiatShamir_RepeatedShuffledDLEQ<SCDLEQSigmaProtocol, ShuffledSigmaProtocol<SCDLEQSigmaProtocol>>(1, 10);
    test_FiatShamir_RepeatedShuffledDLEQ<SCDLEQSigmaProtocol, ShuffledSigmaProtocol<SCDLEQSigmaProtocol>>(32, 10);
    test_FiatShamir_RepeatedShuffledDLEQ<SCDLEQSigmaProtocol, ShuffledSigmaProtocol<SCDLEQSigmaProtocol>>(64, 10);
    test_FiatShamir_RepeatedShuffledDLEQ<SCDLEQSigmaProtocol, ShuffledSigmaProtocol<SCDLEQSigmaProtocol>>(128, 10);
    test_FiatShamir_RepeatedShuffledDLEQ<SCDLEQSigmaProtocol, ShuffledSigmaProtocol<SCDLEQSigmaProtocol>>(128, 100);
    test_FiatShamir_RepeatedShuffledDLEQ<SCDLEQSigmaProtocol, ShuffledSigmaProtocol<SCDLEQSigmaProtocol>>(128, 200);

    test_VEP<SCDLEQSigmaProtocol, ShuffledSigmaProtocol<SCDLEQSigmaProtocol>>(1, 10);
    test_VEP<SCDLEQSigmaProtocol, ShuffledSigmaProtocol<SCDLEQSigmaProtocol>>(32, 10);
    test_VEP<SCDLEQSigmaProtocol, ShuffledSigmaProtocol<SCDLEQSigmaProtocol>>(64, 10);
    test_VEP<SCDLEQSigmaProtocol, ShuffledSigmaProtocol<SCDLEQSigmaProtocol>>(128, 10);
    test_VEP<SCDLEQSigmaProtocol, ShuffledSigmaProtocol<SCDLEQSigmaProtocol>>(128, 100);
    test_VEP<SCDLEQSigmaProtocol, ShuffledSigmaProtocol<SCDLEQSigmaProtocol>>(128, 200);

    printf("All tests passed.\n");
}

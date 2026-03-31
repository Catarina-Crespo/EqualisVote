# Set the minimum version of CMake that can be used
# To find the cmake version run
# $ cmake --version
cmake_minimum_required(VERSION 3.5)

# Added: Specified the standard because I'll be using some C++20 functions
set(CMAKE_CXX_STANDARD 20)

# Set the project name
project (mr_impl)

add_subdirectory(wasm)

# Add an executable
add_executable(mr_impl
    src/main.cpp
    src/utilities.cpp
    src/trivial_zkproof.cpp
    src/dlog.cpp
    src/dlog_to_gen.cpp
    src/dleq.cpp
    src/batched_dleq.cpp
    src/shuffle_compatible_dleq.cpp
    src/fiat_shamir.cpp
    src/base_point.cpp
    src/verifiable_exponentiation.cpp
    src/shuffled_sigma_protocol.cpp
    src/random_permutation.cpp
    src/repeated_sigma_protocol.cpp
    src/keccak.cpp
    src/protocol_run.cpp
    src/parties.cpp
)

# link against libsodium
target_link_libraries(mr_impl PRIVATE sodium)

set(CMAKE_CXX_FLAGS_DEBUG_INIT "-Wall -g")
set(CMAKE_CXX_FLAGS_RELEASE_INIT "-Wall")

add_executable(create_group
        src/utilities.cpp
        src/trivial_zkproof.cpp
        src/dlog.cpp
        src/dlog_to_gen.cpp
        src/dleq.cpp
        src/batched_dleq.cpp
        src/shuffle_compatible_dleq.cpp
        src/fiat_shamir.cpp
        src/base_point.cpp
        src/verifiable_exponentiation.cpp
        src/shuffled_sigma_protocol.cpp
        src/random_permutation.cpp
        src/repeated_sigma_protocol.cpp
        src/keccak.cpp
        src/protocol_run.cpp
        src/parties.cpp)

target_link_libraries(create_group PRIVATE sodium)


add_executable(init_server
        src/utilities.cpp
        src/trivial_zkproof.cpp
        src/dlog.cpp
        src/dlog_to_gen.cpp
        src/dleq.cpp
        src/batched_dleq.cpp
        src/shuffle_compatible_dleq.cpp
        src/fiat_shamir.cpp
        src/base_point.cpp
        src/verifiable_exponentiation.cpp
        src/shuffled_sigma_protocol.cpp
        src/random_permutation.cpp
        src/repeated_sigma_protocol.cpp
        src/keccak.cpp
        src/protocol_run.cpp
        src/parties.cpp)

target_link_libraries(init_server PRIVATE sodium)

add_executable(shuffle_server
        src/modules/shuffle_server.cpp
        src/utilities.cpp
        src/trivial_zkproof.cpp
        src/dlog.cpp
        src/dlog_to_gen.cpp
        src/dleq.cpp
        src/batched_dleq.cpp
        src/shuffle_compatible_dleq.cpp
        src/fiat_shamir.cpp
        src/base_point.cpp
        src/verifiable_exponentiation.cpp
        src/shuffled_sigma_protocol.cpp
        src/random_permutation.cpp
        src/repeated_sigma_protocol.cpp
        src/keccak.cpp
        src/protocol_run.cpp
        src/parties.cpp)

target_link_libraries(shuffle_server PRIVATE sodium)

add_executable(intersection_server
        src/utilities.cpp
        src/trivial_zkproof.cpp
        src/dlog.cpp
        src/dlog_to_gen.cpp
        src/dleq.cpp
        src/batched_dleq.cpp
        src/shuffle_compatible_dleq.cpp
        src/fiat_shamir.cpp
        src/base_point.cpp
        src/verifiable_exponentiation.cpp
        src/shuffled_sigma_protocol.cpp
        src/random_permutation.cpp
        src/repeated_sigma_protocol.cpp
        src/keccak.cpp
        src/protocol_run.cpp
        src/parties.cpp)

target_link_libraries(intersection_server PRIVATE sodium)


add_executable(init_shuffle_server
        src/utilities.cpp
        src/trivial_zkproof.cpp
        src/dlog.cpp
        src/dlog_to_gen.cpp
        src/dleq.cpp
        src/batched_dleq.cpp
        src/shuffle_compatible_dleq.cpp
        src/fiat_shamir.cpp
        src/base_point.cpp
        src/verifiable_exponentiation.cpp
        src/shuffled_sigma_protocol.cpp
        src/random_permutation.cpp
        src/repeated_sigma_protocol.cpp
        src/keccak.cpp
        src/protocol_run.cpp
        src/parties.cpp)

target_link_libraries(init_shuffle_server PRIVATE sodium)

add_executable(w_s_shuffling
        src/utilities.cpp
        src/trivial_zkproof.cpp
        src/dlog.cpp
        src/dlog_to_gen.cpp
        src/dleq.cpp
        src/batched_dleq.cpp
        src/shuffle_compatible_dleq.cpp
        src/fiat_shamir.cpp
        src/base_point.cpp
        src/verifiable_exponentiation.cpp
        src/shuffled_sigma_protocol.cpp
        src/random_permutation.cpp
        src/repeated_sigma_protocol.cpp
        src/keccak.cpp
        src/protocol_run.cpp
        src/parties.cpp)

target_link_libraries(w_s_shuffling PRIVATE sodium)

# -------------- Protocol run (COLBAC) --------------

# Add an executable
add_executable(protocol_run_colbac
        src/protocol_run_colbac.cpp
        src/utilities.cpp
        src/trivial_zkproof.cpp
        src/dlog.cpp
        src/dlog_to_gen.cpp
        src/dleq.cpp
        src/batched_dleq.cpp
        src/shuffle_compatible_dleq.cpp
        src/fiat_shamir.cpp
        src/base_point.cpp
        src/verifiable_exponentiation.cpp
        src/shuffled_sigma_protocol.cpp
        src/random_permutation.cpp
        src/repeated_sigma_protocol.cpp
        src/keccak.cpp
        src/protocol_run.cpp
        src/parties.cpp
)

# link against libsodium
target_link_libraries(protocol_run_colbac PRIVATE sodium)



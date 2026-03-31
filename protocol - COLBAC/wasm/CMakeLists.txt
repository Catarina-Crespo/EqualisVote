cmake_minimum_required(VERSION 3.5)
project(mr_impl_wasm)

set(CMAKE_CXX_STANDARD 20)

include_directories(/root/libsodium-wasm/include)
link_directories(/root/libsodium-wasm/lib)

add_executable(create_user
        ../src/modules/create_user_js.cpp
        ../src/utilities.cpp
        ../src/trivial_zkproof.cpp
        ../src/dlog.cpp
        ../src/dlog_to_gen.cpp
        ../src/dleq.cpp
        ../src/batched_dleq.cpp
        ../src/shuffle_compatible_dleq.cpp
        ../src/fiat_shamir.cpp
        ../src/base_point.cpp
        ../src/verifiable_exponentiation.cpp
        ../src/shuffled_sigma_protocol.cpp
        ../src/random_permutation.cpp
        ../src/repeated_sigma_protocol.cpp
        ../src/keccak.cpp
        ../src/protocol_run.cpp
        ../src/parties.cpp
)

target_link_libraries(create_user PRIVATE sodium)

set_target_properties(create_user PROPERTIES
        LINK_FLAGS "-s WASM=1 -s EXPORT_ES6=1 -s EXPORTED_FUNCTIONS=['_generate_user_keys'] -s MODULARIZE=1 -s EXPORT_NAME=create_user -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap'] -sENVIRONMENT=web,node"
)

add_executable(gen_group_tag
        ../src/modules/gen_group_tag_js.cpp
        ../src/utilities.cpp
        ../src/trivial_zkproof.cpp
        ../src/dlog.cpp
        ../src/dlog_to_gen.cpp
        ../src/dleq.cpp
        ../src/batched_dleq.cpp
        ../src/shuffle_compatible_dleq.cpp
        ../src/fiat_shamir.cpp
        ../src/base_point.cpp
        ../src/verifiable_exponentiation.cpp
        ../src/shuffled_sigma_protocol.cpp
        ../src/random_permutation.cpp
        ../src/repeated_sigma_protocol.cpp
        ../src/keccak.cpp
        ../src/protocol_run.cpp
        ../src/parties.cpp
)

target_link_libraries(gen_group_tag PRIVATE sodium)

set_target_properties(gen_group_tag PROPERTIES
        LINK_FLAGS "-s WASM=1 -s EXPORT_ES6=1 -s EXIT_RUNTIME=1 -s ASSERTIONS=1 -s EXPORTED_FUNCTIONS=['_generate_group_tag'] -s MODULARIZE=1 -s ALLOW_MEMORY_GROWTH=1 -s EXPORT_NAME=gen_group_tag -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap','UTF8ToString','stringToUTF8','lengthBytesUTF8'] -sENVIRONMENT=web,node"
)

add_executable(encrypt_vote
        ../src/modules/encrypt_vote_js.cpp
        ../src/utilities.cpp
        ../src/trivial_zkproof.cpp
        ../src/dlog.cpp
        ../src/dlog_to_gen.cpp
        ../src/dleq.cpp
        ../src/batched_dleq.cpp
        ../src/shuffle_compatible_dleq.cpp
        ../src/fiat_shamir.cpp
        ../src/base_point.cpp
        ../src/verifiable_exponentiation.cpp
        ../src/shuffled_sigma_protocol.cpp
        ../src/random_permutation.cpp
        ../src/repeated_sigma_protocol.cpp
        ../src/keccak.cpp
        ../src/protocol_run.cpp
        ../src/parties.cpp
)

target_link_libraries(encrypt_vote PRIVATE sodium)

set_target_properties(encrypt_vote PROPERTIES
        LINK_FLAGS "-s WASM=1 -s EXPORT_ES6=1 -s EXIT_RUNTIME=1 -s ASSERTIONS=1 -s EXPORTED_FUNCTIONS=['_encrypt_vote'] -s MODULARIZE=1 -s ALLOW_MEMORY_GROWTH=1 -s EXPORT_NAME=encrypt_vote -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap','UTF8ToString','stringToUTF8','lengthBytesUTF8'] -sENVIRONMENT=web,node"
)

add_executable(init_user
        ../src/utilities.cpp
        ../src/trivial_zkproof.cpp
        ../src/dlog.cpp
        ../src/dlog_to_gen.cpp
        ../src/dleq.cpp
        ../src/batched_dleq.cpp
        ../src/shuffle_compatible_dleq.cpp
        ../src/fiat_shamir.cpp
        ../src/base_point.cpp
        ../src/verifiable_exponentiation.cpp
        ../src/shuffled_sigma_protocol.cpp
        ../src/random_permutation.cpp
        ../src/repeated_sigma_protocol.cpp
        ../src/keccak.cpp
        ../src/protocol_run.cpp
        ../src/parties.cpp
)

target_link_libraries(init_user PRIVATE sodium)

set_target_properties(init_user PROPERTIES
        LINK_FLAGS "-s WASM=1 -s EXPORT_ES6=1 -s EXIT_RUNTIME=1 -s ASSERTIONS=1 -s EXPORTED_FUNCTIONS=['_init_user'] -sSTACK_SIZE=8388608 -s MODULARIZE=1 -s ALLOW_MEMORY_GROWTH=1 -s EXPORT_NAME=init_user -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap','UTF8ToString','stringToUTF8','lengthBytesUTF8'] -sENVIRONMENT=web,node"
)

add_executable(shuffle_user
        ../src/modules/shuffle_user_js.cpp
        ../src/utilities.cpp
        ../src/trivial_zkproof.cpp
        ../src/dlog.cpp
        ../src/dlog_to_gen.cpp
        ../src/dleq.cpp
        ../src/batched_dleq.cpp
        ../src/shuffle_compatible_dleq.cpp
        ../src/fiat_shamir.cpp
        ../src/base_point.cpp
        ../src/verifiable_exponentiation.cpp
        ../src/shuffled_sigma_protocol.cpp
        ../src/random_permutation.cpp
        ../src/repeated_sigma_protocol.cpp
        ../src/keccak.cpp
        ../src/protocol_run.cpp
        ../src/parties.cpp
)

target_link_libraries(shuffle_user PRIVATE sodium)

set_target_properties(shuffle_user PROPERTIES
        LINK_FLAGS "-s WASM=1 -s EXPORT_ES6=1 -s EXIT_RUNTIME=1 -s EXPORTED_FUNCTIONS=['_shuffle_user'] -s MODULARIZE=1 -s ALLOW_MEMORY_GROWTH=1 -s EXPORT_NAME=shuffle_user -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap','UTF8ToString','stringToUTF8','lengthBytesUTF8'] -sENVIRONMENT=web,node -g -s ASSERTIONS=2 -sSTACK_SIZE=2097152 -O2"
)

add_executable(group_init_exp
        ../src/modules/group_init_exp_js.cpp
        ../src/utilities.cpp
        ../src/trivial_zkproof.cpp
        ../src/dlog.cpp
        ../src/dlog_to_gen.cpp
        ../src/dleq.cpp
        ../src/batched_dleq.cpp
        ../src/shuffle_compatible_dleq.cpp
        ../src/fiat_shamir.cpp
        ../src/base_point.cpp
        ../src/verifiable_exponentiation.cpp
        ../src/shuffled_sigma_protocol.cpp
        ../src/random_permutation.cpp
        ../src/repeated_sigma_protocol.cpp
        ../src/keccak.cpp
        ../src/protocol_run.cpp
        ../src/parties.cpp
)

target_link_libraries(group_init_exp PRIVATE sodium)

set_target_properties(group_init_exp PROPERTIES
        LINK_FLAGS "-s WASM=1 -s EXPORT_ES6=1 -s EXIT_RUNTIME=1 -s ASSERTIONS=1 -s EXPORTED_FUNCTIONS=['_group_init_exp'] -s MODULARIZE=1 -s ALLOW_MEMORY_GROWTH=1 -s EXPORT_NAME=group_init_exp -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap','UTF8ToString','stringToUTF8','lengthBytesUTF8'] -sENVIRONMENT=web,node"
)


add_executable(intersection
        ../src/modules/intersection_server_js.cpp
        ../src/utilities.cpp
        ../src/trivial_zkproof.cpp
        ../src/dlog.cpp
        ../src/dlog_to_gen.cpp
        ../src/dleq.cpp
        ../src/batched_dleq.cpp
        ../src/shuffle_compatible_dleq.cpp
        ../src/fiat_shamir.cpp
        ../src/base_point.cpp
        ../src/verifiable_exponentiation.cpp
        ../src/shuffled_sigma_protocol.cpp
        ../src/random_permutation.cpp
        ../src/repeated_sigma_protocol.cpp
        ../src/keccak.cpp
        ../src/protocol_run.cpp
        ../src/parties.cpp
)

target_link_libraries(intersection PRIVATE sodium)

set_target_properties(intersection PROPERTIES
        LINK_FLAGS "-fsanitize=address -s WASM=1 -s EXPORT_ES6=1 -s EXIT_RUNTIME=1 -s ASSERTIONS=1 -s EXPORTED_FUNCTIONS=['_intersection'] -s MODULARIZE=1 -s ALLOW_MEMORY_GROWTH=1 -s EXPORT_NAME=intersection -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap','UTF8ToString','stringToUTF8','lengthBytesUTF8'] -sENVIRONMENT=web,node -sNO_DISABLE_EXCEPTION_CATCHING"
)


add_executable(create_election
        ../src/modules/create_election_js.cpp
        ../src/utilities.cpp
        ../src/trivial_zkproof.cpp
        ../src/dlog.cpp
        ../src/dlog_to_gen.cpp
        ../src/dleq.cpp
        ../src/batched_dleq.cpp
        ../src/shuffle_compatible_dleq.cpp
        ../src/fiat_shamir.cpp
        ../src/base_point.cpp
        ../src/verifiable_exponentiation.cpp
        ../src/shuffled_sigma_protocol.cpp
        ../src/random_permutation.cpp
        ../src/repeated_sigma_protocol.cpp
        ../src/keccak.cpp
        ../src/protocol_run.cpp
        ../src/parties.cpp
)

target_link_libraries(create_election PRIVATE sodium)

set_target_properties(create_election PROPERTIES
        LINK_FLAGS "-s WASM=1 -s EXPORT_ES6=1 -s EXIT_RUNTIME=1 -s ASSERTIONS=1 -s EXPORTED_FUNCTIONS=['_create_election'] -s MODULARIZE=1 -s ALLOW_MEMORY_GROWTH=1 -s EXPORT_NAME=create_election -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap','UTF8ToString','stringToUTF8','lengthBytesUTF8'] -sENVIRONMENT=web,node -sNO_DISABLE_EXCEPTION_CATCHING"
)

add_executable(crypto
        ../src/modules/crypto_wasm.cpp
        ../src/utilities.cpp
        ../src/trivial_zkproof.cpp
        ../src/dlog.cpp
        ../src/dlog_to_gen.cpp
        ../src/dleq.cpp
        ../src/batched_dleq.cpp
        ../src/shuffle_compatible_dleq.cpp
        ../src/fiat_shamir.cpp
        ../src/base_point.cpp
        ../src/verifiable_exponentiation.cpp
        ../src/shuffled_sigma_protocol.cpp
        ../src/random_permutation.cpp
        ../src/repeated_sigma_protocol.cpp
        ../src/keccak.cpp
        ../src/protocol_run.cpp
        ../src/parties.cpp
)

target_link_libraries(crypto PRIVATE sodium)

set_target_properties(crypto PROPERTIES
        LINK_FLAGS "-s WASM=1 --bind -s EXPORT_ES6=1 -s EXIT_RUNTIME=1 -s ASSERTIONS=1 -s MODULARIZE=1 -s ALLOW_MEMORY_GROWTH=1 -s EXPORT_NAME=crypto -s -sENVIRONMENT=web,node -sNO_DISABLE_EXCEPTION_CATCHING"
)
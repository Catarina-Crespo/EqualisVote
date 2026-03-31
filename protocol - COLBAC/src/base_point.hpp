#pragma once

#ifdef __cplusplus
extern "C"{
#endif
#include <sodium.h>
#ifdef __cplusplus
}
#endif

// for some reason, libsodium does not expose the ristretto base point B
// we take ours from the following test file:
// https://github.com/jedisct1/libsodium/blob/cd6b337b370495f7c1bd98cfd13e927cbd522dc6/test/default/scalarmult_ristretto255.c
void ristretto255_base_point(unsigned char base[crypto_core_ristretto255_BYTES]);
void ristretto255_identity_point(unsigned char identity[crypto_core_ristretto255_BYTES]);
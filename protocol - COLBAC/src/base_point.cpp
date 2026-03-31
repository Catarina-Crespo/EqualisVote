#include "base_point.hpp"
#include <cassert>
#include <cstring>

#define ristretto255_base_hex "e2f2ae0a6abc4e71a884a961c500515f58e30b6aa582dd8db6a65945e08d2d76"
#define ristretto255_identity_hex "0000000000000000000000000000000000000000000000000000000000000000"

void ristretto255_base_point(unsigned char base[crypto_core_ristretto255_BYTES])
{
    if (sodium_hex2bin(
        base,
        crypto_core_ristretto255_BYTES,
        ristretto255_base_hex,
        sizeof ristretto255_base_hex - (size_t) 1U,
        NULL, NULL, NULL) != 0)
    {
        assert(false);
    }
}

void ristretto255_identity_point(unsigned char identity[crypto_core_ristretto255_BYTES])
{
    memset(identity, 0, crypto_core_ristretto255_BYTES);
}
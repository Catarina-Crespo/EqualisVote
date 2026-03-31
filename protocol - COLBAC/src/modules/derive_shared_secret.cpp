//
// Created by Catarina on 20/01/2026.
//
#include <jni.h>
#include <sodium.h>
#include <vector>
#include <string>
#include <iostream>

static std::vector<unsigned char> from_hex_checked(
        const std::string& hex,
        bool& ok
) {
    std::vector<unsigned char> out(hex.size() / 2);
    size_t bin_len = 0;

    ok = (sodium_hex2bin(
        out.data(),
        out.size(),
        hex.c_str(),
        hex.size(),
        nullptr,
        &bin_len,
        nullptr
    ) == 0);

    if (!ok || bin_len != out.size()) {
        out.clear();
    }
    return out;
}

static std::string to_hex(const unsigned char* data, size_t len) {
    std::string hex(len * 2, '\0');
    sodium_bin2hex(hex.data(), hex.size() + 1, data, len);
    return hex;
}

extern "C" {

JNIEXPORT jstring JNICALL
Java_com_example_CryptoNative_deriveSharedSecretHex(
        JNIEnv* env,
        jclass,
        jstring u_i_hex_j,
        jstring peer_upk_hex_j
) {
    fprintf(stderr, "JNI reached\n");
    fflush(stderr);
std::cerr << "[DEBUG] JNI reached" << std::endl;

    static bool sodium_ready = false;
    if (!sodium_ready) {
        if (sodium_init() < 0) {
            env->ThrowNew(env->FindClass("java/lang/IllegalStateException"),
                          "libsodium init failed");
            return nullptr;
        }
        sodium_ready = true;
    }




    const char* u_i_hex_c = env->GetStringUTFChars(u_i_hex_j, nullptr);
    const char* peer_hex_c = env->GetStringUTFChars(peer_upk_hex_j, nullptr);

    std::string u_i_hex(u_i_hex_c);
    std::string peer_hex(peer_hex_c);

    env->ReleaseStringUTFChars(u_i_hex_j, u_i_hex_c);
    env->ReleaseStringUTFChars(peer_upk_hex_j, peer_hex_c);

    bool ok1 = false, ok2 = false;
    auto u_i = from_hex_checked(u_i_hex, ok1);
    auto peer = from_hex_checked(peer_hex, ok2);

    fprintf(stderr, "Got to conversion\n");
    fflush(stderr);
std::cerr << "[DEBUG] Got to conversion" << std::endl;


    if (!ok1 || !ok2 ||
        u_i.size() != crypto_core_ristretto255_SCALARBYTES ||
        peer.size() != crypto_core_ristretto255_BYTES) {

        env->ThrowNew(env->FindClass("java/lang/IllegalArgumentException"),
                      "Invalid hex input");
        return nullptr;
    }

    fprintf(stderr, "About to validate ristretto\n");
    fflush(stderr);
std::cerr << "[DEBUG] About to validate ristretto" << std::endl;


    // 🔥 CRITICAL: validate Ristretto point
    if (crypto_core_ristretto255_is_valid_point(peer.data()) != 1) {
        env->ThrowNew(env->FindClass("java/lang/IllegalArgumentException"),
                      "peerUpk is NOT a valid Ristretto point");
        return nullptr;
    }

    unsigned char out[crypto_core_ristretto255_BYTES];

fprintf(stderr, "About to multiplicate ristretto\n");
    fflush(stderr);
std::cerr << "[DEBUG] About to multiplicate ristretto" << std::endl;

    if (crypto_scalarmult_ristretto255(out, u_i.data(), peer.data()) != 0) {
        env->ThrowNew(env->FindClass("java/lang/IllegalStateException"),
                      "Ristretto scalar multiplication failed");
        return nullptr;
    }

fprintf(stderr, "About to return result\n");
    fflush(stderr);
std::cerr << "[DEBUG] About to return result" << std::endl;

    std::string hex_out = to_hex(out, crypto_core_ristretto255_BYTES);
    return env->NewStringUTF(hex_out.c_str());
}

}

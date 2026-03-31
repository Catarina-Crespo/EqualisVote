//
// Created by Catarina on 22/01/2026.
//

#ifdef __cplusplus
extern "C" {
#endif
#include <sodium.h>
#ifdef __cplusplus
}
#endif
#include <jni.h>

#include <cstring>
#include <string>

#include "../dlog_to_gen.hpp"
#include "../batched_dleq.hpp"
#include "../verifiable_exponentiation.hpp"
#include "../utilities.hpp"

extern "C"
JNIEXPORT jobjectArray JNICALL
Java_com_example_GroupNative_createGroup(JNIEnv* env, jclass) {

    // --- libsodium init (JNI-safe) ---
    static bool sodium_ready = false;
    if (!sodium_ready) {
        if (sodium_init() < 0) {
            jclass ex = env->FindClass("java/lang/IllegalStateException");
            env->ThrowNew(ex, "libsodium initialization failed");
            return nullptr;
        }
        sodium_ready = true;
    }

    // --- Protocol code (UNCHANGED SEMANTICS) ---
    VEP<BatchedDLEQProof> ve(NULL);

    unsigned char s_G[crypto_core_ristretto255_SCALARBYTES];
    unsigned char spk_G[crypto_core_ristretto255_BYTES];
    unsigned char spk_proof[DLOG2GenProof::proof_BYTES];

    ve.gen(s_G, spk_G, spk_proof);

    // --- Convert to hex ---
    std::string s_G_hex   = to_hex(s_G, crypto_core_ristretto255_SCALARBYTES);
    std::string spk_G_hex = to_hex(spk_G, crypto_core_ristretto255_BYTES);

    // --- Return String[] { s_G_hex, spk_G_hex } ---
    jclass stringClass = env->FindClass("java/lang/String");
    jobjectArray result = env->NewObjectArray(2, stringClass, nullptr);

    env->SetObjectArrayElement(
        result, 0, env->NewStringUTF(s_G_hex.c_str())
    );
    env->SetObjectArrayElement(
        result, 1, env->NewStringUTF(spk_G_hex.c_str())
    );

    return result;
}

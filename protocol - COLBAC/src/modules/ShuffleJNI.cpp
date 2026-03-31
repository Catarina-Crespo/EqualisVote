//
// Created by Catarina on 21/01/2026.
//
#include <jni.h>
#include <vector>
#include <string>
#include "ShuffleResult.hpp"
#include "../utilities.hpp"  // from_hex, hex_to_bytes
#include <sodium.h>
#include "init_count_jni.hpp"


extern "C" {

JNIEXPORT jobject JNICALL
Java_com_example_ShuffleNative_shuffleT(
    JNIEnv *env,
    jobject,
    jint groupSize,
    jstring sGHex,
    jobjectArray THexArray,
    jint nBallots,
    jobjectArray ballotHexArray
) {
    if (sodium_init() < 0) return nullptr;

    const char *sGStr = env->GetStringUTFChars(sGHex, nullptr);
    auto s_G_vec = from_hex(sGStr);
    env->ReleaseStringUTFChars(sGHex, sGStr);

    unsigned char **T = new unsigned char*[groupSize];
    for (int i = 0; i < groupSize; i++) {
        jstring jT = (jstring) env->GetObjectArrayElement(THexArray, i);
        const char *hex = env->GetStringUTFChars(jT, nullptr);

        auto bytes = from_hex(hex);
        T[i] = (unsigned char*) malloc(bytes.size());
        memcpy(T[i], bytes.data(), bytes.size());

        env->ReleaseStringUTFChars(jT, hex);
        env->DeleteLocalRef(jT);
    }

    std::vector<unsigned char*> ballots;
    for (int i = 0; i < nBallots; i++) {
        jstring jb = (jstring) env->GetObjectArrayElement(ballotHexArray, i);
        const char *hex = env->GetStringUTFChars(jb, nullptr);

        auto bytes = from_hex(hex);
        unsigned char* buf = (unsigned char*) malloc(bytes.size());
        memcpy(buf, bytes.data(), bytes.size());
        ballots.push_back(buf);

        env->ReleaseStringUTFChars(jb, hex);
        env->DeleteLocalRef(jb);
    }

    ShuffleResult r = init_count_jni(
        groupSize,
        s_G_vec.data(),
        T,
        ballots,
        nBallots
    );

    jclass cls = env->FindClass("com/example/models/ShuffleResult");
    if (!cls) {
        env->ExceptionDescribe();
        return nullptr;
    }

    jmethodID ctor = env->GetMethodID(cls, "<init>", "()V");
    jobject obj = env->NewObject(cls, ctor);

    auto set = [&](const char *name, const std::string &v) {
        jfieldID fid = env->GetFieldID(cls, name, "Ljava/lang/String;");
        if (!fid) {
            env->ExceptionDescribe();
            return;
        }
        jstring js = env->NewStringUTF(v.c_str());
        env->SetObjectField(obj, fid, js);
        env->DeleteLocalRef(js);
    };

    set("s_i_star_G", r.s_i_star_G);
    set("overline_spk_i_start_G", r.overline_spk_i_start_G);
    set("overline_spk_proof", r.overline_spk_proof);
    set("Delta_i_star_G", r.Delta_i_star_G);
    set("spk_i_star_G", r.spk_i_star_G);
    set("spk_i_star_proof", r.spk_i_star_proof);
    set("rho_i_star_G", r.rho_i_star_G);
    set("Tprime", r.Tprime);
    set("Ws", r.W_s);

    return obj;
}
}
package com.example;

public class RistrettoDH {
    static { System.loadLibrary("ristretto_dh"); } // load shared library

    // JNI method signature
    public static native void deriveSharedSecret(byte[] privateKey, byte[] peerPublicKey, byte[] out);

    // Convenience wrapper to get the shared secret
    public static byte[] computeSharedSecret(byte[] privateKey, byte[] peerPublicKey) {
        byte[] secret = new byte[32];
        deriveSharedSecret(privateKey, peerPublicKey, secret);
        return secret;
    }
}

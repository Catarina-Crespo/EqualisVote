package com.example;

public class CryptoNative {

    static {
        System.loadLibrary("libsodium-26");
        System.loadLibrary("secret");
    }

    public static native String deriveSharedSecretHex(
            String u_i_hex,
            String peerUpk_hex
    );
}

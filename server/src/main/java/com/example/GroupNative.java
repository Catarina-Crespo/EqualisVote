package com.example;


public class GroupNative {

    static {
        System.loadLibrary("libsodium-26");
        System.loadLibrary("createGroup"); // libshuffle.so
    }

    public static native String[] createGroup();
}

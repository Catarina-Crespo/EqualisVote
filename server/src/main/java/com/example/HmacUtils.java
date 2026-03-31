package com.example;

import javax.crypto.Mac;
import javax.crypto.spec.SecretKeySpec;
import java.security.MessageDigest;

public class HmacUtils {

    // Simple HKDF using SHA-256
    public static byte[] hkdfSha256(byte[] sharedSecret, byte[] info, int length) throws Exception {
        // Extract + Expand
        // In this simple example, salt is empty
        byte[] salt = new byte[32];
        Mac mac = Mac.getInstance("HmacSHA256");
        mac.init(new SecretKeySpec(salt, "HmacSHA256"));
        byte[] prk = mac.doFinal(sharedSecret); // pseudo-random key

        // Expand
        mac.init(new SecretKeySpec(prk, "HmacSHA256"));
        byte[] okm = new byte[length];
        byte[] previous = new byte[0];
        int pos = 0;
        int counter = 1;
        while (pos < length) {
            mac.update(previous);
            mac.update(info);
            mac.update((byte) counter);
            previous = mac.doFinal();
            int copyLength = Math.min(previous.length, length - pos);
            System.arraycopy(previous, 0, okm, pos, copyLength);
            pos += copyLength;
            counter++;
        }
        return okm;
    }

    // Compute HMAC-SHA256
    public static byte[] computeHmac(byte[] key, byte[] message) throws Exception {
        Mac mac = Mac.getInstance("HmacSHA256");
        mac.init(new SecretKeySpec(key, "HmacSHA256"));
        return mac.doFinal(message);
    }

    // Verify HMAC-SHA256
    public static boolean verifyHmac(byte[] key, byte[] message, byte[] tag) throws Exception {
        byte[] expected = computeHmac(key, message);

        // Debug output
        System.out.println("Expected HMAC: " + bytesToHex(expected));
        System.out.println("Received HMAC: " + bytesToHex(tag));

        return MessageDigest.isEqual(expected, tag);
    }

    // Helper to convert bytes to hex for debugging
    private static String bytesToHex(byte[] bytes) {
        StringBuilder sb = new StringBuilder();
        for (byte b : bytes) {
            sb.append(String.format("%02x", b));
        }
        return sb.toString();
    }
}

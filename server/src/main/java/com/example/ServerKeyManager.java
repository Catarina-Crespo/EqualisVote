package com.example;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;

import java.io.IOException;
import java.net.*;
import java.net.http.HttpClient;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse;

import static com.example.Utils.THIRD_PARTY_URL;

public final class ServerKeyManager {

    private static volatile boolean initialized = false;

    private static String privateKeyHex;
    private static String publicKeyHex;

    private static String pkThirdParty;

    private ServerKeyManager() {}

    public static synchronized void init() {
        if (initialized) return;

        String[] keys = GroupNative.createGroup();

        privateKeyHex = keys[0];
        publicKeyHex  = keys[1];
        pkThirdParty = null;

        initialized = true;

        System.out.println("Server keypair initialized");
    }

    public static String getPrivateKeyHex() {
        ensureInit();
        return privateKeyHex;
    }

    public static String getPublicKeyHex() {
        ensureInit();
        return publicKeyHex;
    }

    public static String getThirdPartyPK() {
        return pkThirdParty;
    }

    public static String loadThirdPartyPK() {
        HttpClient client = HttpClient.newHttpClient();
        HttpRequest request = HttpRequest.newBuilder()
                .uri(URI.create(THIRD_PARTY_URL + "/key"))
                .GET()
                .build();
        try {
            HttpResponse<String> response = client.send(request, HttpResponse.BodyHandlers.ofString());

            System.out.println("Status code: " + response.statusCode());
            System.out.println("Response body: " + response.body());

            if (response.statusCode() == 200) {
                ObjectMapper mapper = new ObjectMapper();
                JsonNode root = mapper.readTree(response.body());

                String pk = root.get("pk").asText();
                System.out.println("Extracted pk: " + pk);

                pkThirdParty = pk;
                return pkThirdParty;
            }
        }
        catch (IOException | InterruptedException e) {
            e.printStackTrace();
        }
        return null;
    }

    private static void ensureInit() {
        if (!initialized) {
            throw new IllegalStateException("ServerKeyManager not initialized");
        }
    }

    public static void destroy() {
        if (!initialized) return;
        privateKeyHex = null;
        publicKeyHex = null;
        initialized = false;
    }
}

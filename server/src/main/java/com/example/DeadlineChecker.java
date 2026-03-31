package com.example;

import com.example.db.DBClient;
import com.example.db.NewManager;
import com.example.models.Election;
import com.example.notifications.TagListNotification;
import com.example.websockets.WebSocketSessionManager;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.node.ObjectNode;
import jakarta.servlet.ServletContextEvent;
import jakarta.servlet.ServletContextListener;
import jakarta.servlet.annotation.WebListener;
import jakarta.websocket.Session;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.time.Instant;
import java.util.List;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

import static com.example.Utils.*;

@WebListener
public class DeadlineChecker implements ServletContextListener {

    private ScheduledExecutorService scheduler;

    @Override
    public void contextInitialized(ServletContextEvent sce) {
        scheduler = Executors.newSingleThreadScheduledExecutor();

        scheduler.scheduleAtFixedRate(() -> {
            try {
                checkDeadlines();
            } catch (Exception e) {
                e.printStackTrace();
            }
            //}, 0, 1, TimeUnit.MINUTES); // Run every minute
        }, 0, 30, TimeUnit.SECONDS);
    }

    private void checkDeadlines() throws IOException {
        DBClient db = NewManager.getInstance();

        List<Election> expiredElections = db.getElectionsForIntersection(Instant.now());

        for (Election election : expiredElections) {
            // Send websocket notification to a group user, starting the intersection

            // Find an available group user
            Session groupUserSession = null;

            String spk = election.getGroup_spk();
            List<String> groupUsers = db.getGroupUsersUpks(spk);
            String groupUserUPK = null;

            for (String user : groupUsers) {
                if (WebSocketSessionManager.isOnline(user)) {
                    // Check the Websocket open sessions to see if some of them belongs to a user of the group
                    groupUserSession = WebSocketSessionManager.getSession(user);
                    groupUserUPK = user;
                    System.out.println("User " + user + " had an open WS session" );
                    break;
                }
            }

            // If the user is not online
            if (groupUserSession == null || !groupUserSession.isOpen()) {
                System.out.println("No users online - Impossible to calculate the result of the election.");
            } else {
                List<String> groupTags = db.getGroup(spk).getTags();
                String tags = convertIntoString(groupTags);

                ObjectMapper mapper = new ObjectMapper();

                ObjectNode payloadNode = mapper.valueToTree(new TagListNotification(
                        INT_STEP_1_C,
                        tags,
                        election.getEpk_E(),
                        groupTags.size(),
                        election.getGroup_spk()
                ));

                // Serialize payload for HMAC
                String payloadString = mapper.writeValueAsString(payloadNode);

                // ------------ HMAC Computation [START] -------------
                // Calculate Shared Secret
                String sharedSecretHex = CryptoNative.deriveSharedSecretHex(ServerKeyManager.getPrivateKeyHex(), groupUserUPK);

                System.out.println("Printing shared secret: " + sharedSecretHex);
                // Compute HMAC
                byte[] bPayload = payloadString.getBytes(StandardCharsets.UTF_8);

                byte[] macBytes = new byte[0];
                try {
                    macBytes = HmacUtils.computeHmac(
                            fromHexToByte(sharedSecretHex),
                            bPayload
                    );
                } catch (Exception e) {
                    e.printStackTrace();
                }

                String macHex = Utils.toHex(macBytes);
                // ------------ HMAC Computation [END] -------------

                // Wrap in envelope
                ObjectNode notify = mapper.createObjectNode();
                notify.put("hmac", macHex);
                notify.set("payload", payloadNode);

                // Send notification with HMAC
                System.out.println("Started intersection for election " + election.getEpk_E().substring(0,6) + "...");
                System.out.println("Sending notification: " + notify.toString());
                groupUserSession.getBasicRemote().sendText(notify.toString());

            }

            // Should be marked as done or something
        }
    }

    @Override
    public void contextDestroyed(ServletContextEvent sce) {
        scheduler.shutdownNow();
    }
}

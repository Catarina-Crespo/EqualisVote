package com.example.servlets;

import com.example.CryptoNative;
import com.example.HmacUtils;
import com.example.ServerKeyManager;
import com.example.Utils;
import com.example.db.NewManager;
import com.example.models.Election;
import com.example.db.DBClient;
import com.example.models.Group;
import com.example.models.User;
import com.example.notifications.ElectionNotification;
import com.example.notifications.Notification;
import com.example.websockets.WebSocketSessionManager;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.node.ObjectNode;
import jakarta.servlet.annotation.WebServlet;
import jakarta.servlet.http.HttpServlet;
import jakarta.servlet.http.HttpServletRequest;
import jakarta.servlet.http.HttpServletResponse;
import jakarta.websocket.Session;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.List;
import java.util.stream.Collectors;

import static com.example.ServerKeyManager.getThirdPartyPK;
import static com.example.ServerKeyManager.loadThirdPartyPK;
import static com.example.Utils.*;

@WebServlet(name = "CreateElectionServlet", urlPatterns = {CREATE_ELECTION_URL})
public class CreateElectionServlet extends HttpServlet {

    @Override
    protected void doPost(HttpServletRequest request, HttpServletResponse response)
            throws IOException {

        System.out.println("[Create Election Servlet]");

        // Call DB instance
        DBClient db = NewManager.getInstance();

        // Retrieve Third party's public key to compute the HMAC
        String clientPK = getThirdPartyPK();
        if (clientPK == null) clientPK = loadThirdPartyPK();

        // ------------ HMAC Verification [START] -------------
        // Read raw JSON body as string
        String body = request.getReader()
                .lines()
                .collect(Collectors.joining());

        System.out.println("Received payload: ");
        System.out.println(body);

        String mac = request.getHeader("X-MAC");

        System.out.println("Received MAC: " + mac);
        System.out.println("Third Party PK: " + clientPK);

        if (mac == null || clientPK == null) {
            response.setStatus(HttpServletResponse.SC_BAD_REQUEST);
            response.getWriter().println("Missing authentication data");
            return;
        }

        byte[] bContent = body.getBytes(StandardCharsets.UTF_8);

        String sharedSecretHex = CryptoNative.deriveSharedSecretHex(ServerKeyManager.getPrivateKeyHex(), clientPK);

        boolean valid = false;
        try {
            valid = HmacUtils.verifyHmac(fromHexToByte(sharedSecretHex), bContent, fromHexToByte(mac));
        } catch (Exception e) {
            e.printStackTrace();
        }

        if (!valid) {
            response.setStatus(HttpServletResponse.SC_UNAUTHORIZED);
            response.getWriter().println("Invalid HMAC");
            return;
        }
        // ------------ HMAC Verification [END] -------------

        System.out.println("Passed HMAC Verification in Create Election Servlet");

        // ------------ Actual Servlet Logic [START] -------------

        response.setContentType("text/html;charset=UTF-8");
        String message;

        // Parse the JSON body into a RequestData object
        ObjectMapper mapper = new ObjectMapper();
        Election election;
        try {
            // Deserialize the request body into RequestData
            election = mapper.readValue(body, Election.class);

            // Print the data for debugging
            System.out.println("Received request data: ");
            System.out.println("epk_E: " + election.getEpk_E());
            System.out.println("name: " + election.getName());
            System.out.print("options: [");
            for (String option : election.getOptions()) {
                System.out.print(option + ", ");
            }
            System.out.println("]");
            System.out.println("deadline: " + election.getDeadline());
            System.out.println("group_spk: " + election.getGroup_spk());

        } catch (Exception e) {
            System.err.println("Failed to parse JSON:");
            e.printStackTrace();
            response.sendError(HttpServletResponse.SC_BAD_REQUEST, "Invalid JSON format");
            return;
        }

        System.out.println("[C.Election] Election " + election.getEpk_E().substring(0, 6) +
                ".. with the name <" + election.getName() + "> for the group " +
                election.getGroup_spk().substring(0, 6) + ".. was received");


        String spk = election.getGroup_spk();
        String epk = election.getEpk_E();

        System.out.println("Printing spk:" + spk + ":");

        Group group = db.getGroup(spk);



        if (group != null)
            System.out.println("Printing group size: " + group.getUsers().size());
        else
            System.out.println("Group doesn't exist");

        // If the group is invalid or the election exists, ABORT
        // group.getUsers().size() < 2 ||
        if (db.getElection(epk) != null) {
            message = "Group doesn't exist or has less than 2 confirmed users or an election with such key was already created";
            response.sendError(HttpServletResponse.SC_CONFLICT, message);
        }

        // Create the election on the database
        db.addElection(epk, election);
        group.addElection(election);

        // Retrieve the group members
        List<User> members = group.getUsers();

        int count = 0;
        // Add the vote requests to the users' mailbox
        for (User u : members) {

            // Create a notification
            Notification notification = new ElectionNotification(election);

            Session userSession = WebSocketSessionManager.getSession(u.getUpk());
            String notificationS = mapper.writeValueAsString(notification);

            // ------------ HMAC Computation [START] -------------
            String payloadJson = mapper.writeValueAsString(notification);

            // Calculate Shared Secret
            sharedSecretHex = CryptoNative.deriveSharedSecretHex(ServerKeyManager.getPrivateKeyHex(), u.getUpk());

            // Compute HMAC
            byte[] bPayload = payloadJson.getBytes(StandardCharsets.UTF_8);

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
            ObjectNode msg = mapper.createObjectNode();
            msg.put("hmac", macHex);
            msg.set("payload", mapper.valueToTree(notification));


            // If the user is online, send a request via websocket - TODO - check if this shouldn't be changed
            if (userSession != null && userSession.isOpen()) {
                try {
                    userSession.getBasicRemote().sendText(msg.toString());
                    count += 1;
                    System.out.println("Sent real-time voting request to: " + u.getUpk());
                } catch (IOException e) {
                    System.err.println("Failed to send WebSocket notification to " + u.getUpk());
                    e.printStackTrace();
                }
            }
            u.addNotification(notification.getID(), notification);
        }

        System.out.println("Sent " + count + " election invites out of " + members.size());
    }

}

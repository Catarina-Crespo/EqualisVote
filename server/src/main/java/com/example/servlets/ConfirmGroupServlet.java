package com.example.servlets;

import com.example.CryptoNative;
import com.example.HmacUtils;
import com.example.ServerKeyManager;
import com.example.db.DBClient;
import com.example.db.NewManager;
import jakarta.servlet.annotation.WebServlet;
import jakarta.servlet.http.HttpServlet;
import jakarta.servlet.http.HttpServletRequest;
import jakarta.servlet.http.HttpServletResponse;

import java.io.IOException;
import java.nio.charset.StandardCharsets;

import static com.example.Utils.*;

@WebServlet(name = "ConfirmServlet", urlPatterns = {CONFIRM_URL})
public class ConfirmGroupServlet extends HttpServlet {

    @Override
    protected void doPost(HttpServletRequest request, HttpServletResponse response)
            throws IOException {

        System.out.println("[Confirm Group Servlet]");

        // Call DB instance
        DBClient db = NewManager.getInstance();


        // ------------ HMAC Verification [START] -------------
        String expectedContent = request.getQueryString();

        String upk = request.getParameter("upk");
        String mac = request.getHeader("X-MAC");

        if (mac == null || expectedContent == null || upk == null) {
            response.setStatus(HttpServletResponse.SC_BAD_REQUEST);
            response.getWriter().println("Missing authentication data");
            return;
        }

        byte[] bContent = expectedContent.getBytes(StandardCharsets.UTF_8);

        String sharedSecretHex = CryptoNative.deriveSharedSecretHex(ServerKeyManager.getPrivateKeyHex(), upk);

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


        // ------------ Actual Servlet Logic [START] -------------

        response.setContentType("text/html;charset=UTF-8");
        String message;

        // Retrieve parameters from the URL
        String spk = request.getParameter("spk");
        String tag = request.getParameter("tag");
        String id = request.getParameter("id");

        // Remove the invite from the user's mailbox
        db.getUser(upk).removeNotification(id);

        if (tag.equals("null")) {
            System.out.println("The invitation was rejected");
            response.getWriter().println("<h1>The invitation was rejected</h1>");

        } else {
            System.out.println("[Conf.Group] User " + upk.substring(0, 6) +
                    ".. submitted the tag " + tag.substring(0, 6) + ".. for the group " +
                    spk.substring(0, 6));

            // Check if the invite is valid (if user <upk> was invited to group <spk>
            if (db.isGroupInviteValid(spk, upk)) {
                db.addUserToGroup(upk, spk, tag);
                message = "The invitation was confirmed and the user " + upk.substring(0, 6) + ".. added to the group";

                int groupSize = db.getGroup(spk).getUsers().size();
                System.out.println("The group has now " + groupSize + " members confirmed");


            } else {
                System.out.println("The group invite was invalid (upk or spk were wrong)");
                message = "The group invite was invalid (upk or spk were wrong)";

                response.sendError(HttpServletResponse.SC_INTERNAL_SERVER_ERROR, "Group not ready");
            }
            response.getWriter().println(message);
        }
    }
}

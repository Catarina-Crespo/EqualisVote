import axios from "axios";
import WebSocket from "ws";

axios.defaults.baseURL = "http://localhost:3000";
axios.defaults.withCredentials = true;

let openSockets = [];
let votes = [];
let alpha = null;
let Tpp = null;

let serverPK = null; // Server public key
let thirdPartyPK = null; // Server public key

// WASM loader
async function loadWasmFunctions() {
  const createUserMod = await import("./binaries/create_user.js");
  const tagMod = await import("./binaries/gen_group_tag.js");
  const voteMod = await import("./binaries/encrypt_vote.js");
  const groupInitMod = await import("./binaries/group_init_exp.js");
  const shuffleMod = await import("./binaries/shuffle_user.js");
  const intersectionMod = await import("./binaries/intersection.js");
  const cryptoMod = await import("./binaries/crypto.js");

  const [user, tag, vote, init, shuffle, intersection, crypto] =
    await Promise.all([
      createUserMod.default(),
      tagMod.default(),
      voteMod.default(),
      groupInitMod.default(),
      shuffleMod.default(),
      intersectionMod.default(),
      cryptoMod.default(),
    ]);

  return {
    generateKeys: user.cwrap("generate_user_keys", "string", []),
    generateTag: tag.cwrap("generate_group_tag", "string", [
      "string",
      "string",
      "string",
    ]),
    encryptVote: vote.cwrap("encrypt_vote", "string", [
      "string",
      "string",
      "int",
    ]),
    groupInitExp: init.cwrap("group_init_exp", "string", ["int", "string"]),
    shuffleUser: shuffle.cwrap("shuffle_user", "string", [
      "string",
      "string",
      "string",
    ]),
    intersection: intersection.cwrap("intersection", "string", [
      "int",
      "string",
      "string",
      "string",
      "string",
      "int",
      "int",
      "int",
    ]),
    // ---- Embind-based crypto module ----
    crypto: {
      deriveSharedSecretHex: crypto.derive_shared_secret_hex,
      createHmacHex: crypto.create_hmac_hex,
      verifyHmacHex: crypto.verify_hmac_hex,
    },
  };
}

async function loadKeys() {
  const res = await axios.get("/keys");
  thirdPartyPK = res.data.thirdPartyPK;
  serverPK = res.data.serverPK;
  console.log("Keys loaded");
}

// Function to create a client and send the public key to the server
async function createClient(wasm) {
  const keys = JSON.parse(wasm.generateKeys());

  await axios.post("/send-user-key", {
    browser: false,
    u_i: keys.u_i,
    upk: keys.upk_u_i,
  });

  return {
    ...keys,
    tag: null,
  };
}

// Function to send a request to the server to create a group and receive the group ID, spk
async function createGroup(creator, members) {
  const upks = members.map((c) => c.upk_u_i);
  const res = await axios.post("/request-group-creation", {
    browser: false,
    name: "Some name",
    u_i: creator.u_i,
    upk: creator.upk_u_i,
    members: upks,
  });
  return { spk_G: res.data.spk_G, s_G: res.data.s_G };
}

// Function to vote on an user randomly with a value between 1 and 10
async function voteForUser(voter, votee_upk, wasm, groupUsers) {
  const score = Math.floor(Math.random() * 10) + 1;
  if (groupUsers.includes(voter)) votes.push(score);
  const ballot = JSON.parse(
    wasm.encryptVote(voter.v_i, votee_upk, score),
  ).ballot;

  await axios.post("/send-vote", {
    browser: false,
    u_i: voter.u_i,
    upk: voter.upk_u_i,
    votee: votee_upk,
    ballot,
  });
}

// Function to start a join request
async function requestJoinGroup(client, spk_G, wasm) {
  const tag = JSON.parse(
    wasm.generateTag(client.upk_u_i, client.v_i, spk_G),
  ).z_i_G;

  await axios.post("/join-group", {
    browser: false,
    tag,
    spk_G,
    upk: client.upk_u_i,
    u_i: client.u_i,
  });
}

async function retryConfirmGroup(
  spk,
  tag,
  upk,
  u_i,
  id,
  retries = 15,
  delayMs = 1000,
) {
  for (let i = 0; i < retries; i++) {
    try {
      await axios.post("/confirm-group", {
        browser: false,
        spk,
        tag,
        upk,
        u_i,
        id,
      });
      console.log(
        `Confirmed group ${spk.slice(0, 10)} for user ${upk.slice(0, 10)}`,
      );
      return;
    } catch (err) {
      console.warn(
        `Retry ${i + 1} failed for ${upk.slice(0, 10)}: ${
          err.response?.status || err.message
        }`,
      );
      await new Promise((res) => setTimeout(res, delayMs));
    }
  }
  throw new Error(
    `Failed to confirm group for user ${upk.slice(0, 10)} after retries`,
  );
}

// Function to automatically confirm group invite
function autoConfirmGroupInvite(user, spk_G, wasm) {
  return new Promise((resolve, reject) => {
    const { upk_u_i, v_i, u_i } = user;
    const ws = new WebSocket(`ws://localhost:8080/server_war/ws/${upk_u_i}`);

    ws.on("open", () => {
      console.log(
        `WS opened for group user: ${upk_u_i.slice(
          0,
          10,
        )}... Waiting for group invites...`,
      );
    });

    ws.on("message", async (data) => {
      const fullMsg = JSON.parse(data);
      console.log(
        `User ${user.upk_u_i.slice(0, 10)} received message:`,
        fullMsg,
      );

      // ------------ HMAC Verification [START] -------------
      const { hmac, payload } = fullMsg;
      // Get upk of the sender (Tomcat server)

      const sharedSecret = wasm.crypto.deriveSharedSecretHex(
        user.u_i,
        serverPK,
      );

      console.log("Printing shared secret: " + sharedSecret);

      // Verify
      let msg = JSON.stringify(payload);

      const valid = wasm.crypto.verifyHmacHex(sharedSecret, msg, hmac);

      if (!valid) {
        console.error("Invalid HMAC - message rejected");
        return;
      }

      msg = payload;

      // ------------ HMAC Verification [END] -------------


      if (msg.type === "group_invite") {
        const spk = msg.groupId;
        const tag = JSON.parse(wasm.generateTag(upk_u_i, v_i, spk)).z_i_G;
        user.tag = tag;

        ws.send(
          JSON.stringify({
            type: "ack",
            upk: upk_u_i,
          }),
        );
        console.log(
          `User ${upk_u_i.slice(0, 10)} sent group acknowledgement: `,
        );

        try {
          await retryConfirmGroup(spk_G, tag, upk_u_i, u_i, msg.ID);
        } catch (err) {
          console.error("Confirm failed:", err.message);
        }

        ws.close();
        resolve();
      }

      ws.on("error", reject);
      ws.on("close", () => {});
    });
  });
}

// Function to Open a WebSocket connection
function openWebSocketConnection(user, spk_G, wasm) {
  return new Promise((resolve, reject) => {
    const ws = new WebSocket(
      `ws://localhost:8080/server_war/ws/${user.upk_u_i}`,
    );

    ws.on("open", () => {
      console.log(`WebSocket opened for user ${user.upk_u_i.slice(0, 10)}`);
      user.websocket = ws;
      openSockets.push[ws];
      resolve();
    });

    ws.on("message", async (data) => {
      //console.log("Received: " + data);
      const fullMsg = JSON.parse(data);
      console.log(
        `User ${user.upk_u_i.slice(0, 10)} received message:`,
        fullMsg,
      );

      // ------------ HMAC Verification [START] -------------
      const { hmac, payload } = fullMsg;
      // Get upk of the sender (Tomcat server)

      const sharedSecret = wasm.crypto.deriveSharedSecretHex(
        user.u_i,
        serverPK,
      );

      console.log("Printing shared secret: " + sharedSecret);

      // Verify
      let msg = JSON.stringify(payload);

      const valid = wasm.crypto.verifyHmacHex(sharedSecret, msg, hmac);

      if (!valid) {
        console.error("Invalid HMAC - message rejected");
        return;
      }

      msg = payload;

      // ------------ HMAC Verification [END] -------------

      switch (msg.type) {
        case "int_step_1": {
          const result = JSON.parse(
            wasm.groupInitExp(msg.groupSize, msg.tagList),
          );
          alpha = result.alpha;

          //console.log("Group Init Exp done with result: ");
          //console.log(result);
          //console.log("Sending int_step_1_reply to the server");

          const payload = {
            type: "int_step_1_reply",
            T: result.T,
            upk: msg.upk,
            group_size: msg.groupSize,
            spk: msg.spk,
          };

          const payloadString = JSON.stringify(payload);

          // ------------ HMAC Computation [START] -------------
          const sharedSecret = wasm.crypto.deriveSharedSecretHex(
            user.u_i,
            serverPK,
          );

          const hmac = wasm.crypto.createHmacHex(sharedSecret, payloadString);
          // ------------ HMAC Computation [END] -------------

          const envelope = {
            hmac,
            payload, // object form for convenience
          };

          ws.send(JSON.stringify(envelope));
          console.log("Sent " + JSON.stringify(envelope));

          //console.log("Sent");
          break;
        }
        case "int_step_2": {
          const result = JSON.parse(
            wasm.shuffleUser(user.u_i, user.upk_u_i, msg.Tprime),
          );
          //console.log("Logging result:");
          //console.log(result);
          Tpp = result.Tpp;

          const payload = {
            type: "int_step_2_reply",
            Tpp,
            upk: msg.upk,
            group_size: msg.groupSize,
            spk: msg.spk,
          };

          const payloadString = JSON.stringify(payload);

          // ------------ HMAC Computation [START] -------------
          const sharedSecret = wasm.crypto.deriveSharedSecretHex(
            user.u_i,
            serverPK,
          );

          const hmac = wasm.crypto.createHmacHex(sharedSecret, payloadString);
          // ------------ HMAC Computation [END] -------------

          const envelope = {
            hmac,
            payload, // object form for convenience
          };

          ws.send(JSON.stringify(envelope));

          break;
        }
        case "int_step_3": {
          const result = JSON.parse(
            wasm.intersection(
              msg.groupSize,
              alpha,
              msg.Ws,
              msg.overline_spk_i_start_G,
              Tpp,
              msg.minVote,
              msg.maxVote,
              msg.nBallots,
            ),
          );

          console.log("Printing result of intersection: " + result.X);
          console.log("Printing original votes: " + votes);

          const payload = {
            type: "int_step_3_reply",
            X: result.X,
            upk: msg.upk,
            spk: msg.spk,
          };

          const payloadString = JSON.stringify(payload);

          // ------------ HMAC Computation [START] -------------
          const sharedSecret = wasm.crypto.deriveSharedSecretHex(
            user.u_i,
            serverPK,
          );

          const hmac = wasm.crypto.createHmacHex(sharedSecret, payloadString);
          // ------------ HMAC Computation [END] -------------

          const envelope = {
            hmac,
            payload, // object form for convenience
          };

          ws.send(JSON.stringify(envelope));
          break;
        }
        case "int_step_4": {
          const result = JSON.parse(
            wasm.generateTag(user.upk_u_i, user.v_i, msg.spk),
          );

          const payload = {
            type: "int_step_4_reply",
            tag: result.z_i_G,
            upk: msg.upk,
            spk: msg.spk,
          };

          const payloadString = JSON.stringify(payload);

          // ------------ HMAC Computation [START] -------------
          const sharedSecret = wasm.crypto.deriveSharedSecretHex(
            user.u_i,
            serverPK,
          );

          const hmac = wasm.crypto.createHmacHex(sharedSecret, payloadString);
          // ------------ HMAC Computation [END] -------------

          const envelope = {
            hmac,
            payload, // object form for convenience
          };

          ws.send(JSON.stringify(envelope));
          break;
        }
        case "int_step_5": {
          console.log(msg.message);

          // Close all open WebSocket connections
          openSockets.forEach((socket) => {
            if (socket.readyState === WebSocket.OPEN) {
              socket.close();
            }
          });

          console.log("[CHECKPOINT] Simulation complete!");
          process.exit(0);
          break;
        }
        default:
          console.warn("Unhandled WS msg (external user):", msg.type);
      }
    });

    ws.on("error", (err) => {
      console.error(`WebSocket error for ${user.upk_u_i.slice(0, 10)}:`, err);
      reject(err);
    });

    ws.on("close", () => {
      console.log(`WebSocket closed for user ${user.upk_u_i.slice(0, 10)}`);
    });
  });
}

// ---------------------------------------------------------------------------

// ---------------------------- [SIMULATION] -----------------------------------

async function simulateFiveClients() {
  const wasm = await loadWasmFunctions();

  // Load keys for HMAC
  loadKeys();

  console.log("Creating the clients...");
  // Creates five clients
  const clients = await Promise.all([
    createClient(wasm),
    createClient(wasm),
    createClient(wasm),
    createClient(wasm),
    createClient(wasm),
  ]);
  console.log("[CHECKPOINT] All clients created");

  // Just for easier recognition
  const [creator, c2, c3, externalUser, externalVoter] = clients;
  const groupMembers = [creator, c2, c3];

  // One of the clients creates the group with other two users
  const { spk_G } = await createGroup(creator, groupMembers);
  console.log("[CHECKPOINT] Created group with ID: " + spk_G);

  await new Promise((res) => setTimeout(res, 1000));

  // All the clients must confirm the invitation
  await Promise.all(
    groupMembers.map((c) => autoConfirmGroupInvite(c, spk_G, wasm)),
  );
  console.log("[CHECKPOINT] All users confirmed the group invitation");

  groupMembers.push(externalUser);

  // Connect all clients via websocket
  console.log("Connecting via WebSocket...");
  await Promise.all(
    groupMembers.map((c) => openWebSocketConnection(c, spk_G, wasm)),
  );
  console.log("[CHECKPOINT] All users have their websockets open");

  // Voting on the external user
  console.log("Submitting votes for the external user...");
  await voteForUser(externalVoter, externalUser.upk_u_i, wasm, groupMembers);
  await voteForUser(creator, externalUser.upk_u_i, wasm, groupMembers);
  await voteForUser(c2, externalUser.upk_u_i, wasm, groupMembers);
  await voteForUser(c3, externalUser.upk_u_i, wasm, groupMembers);
  console.log("[CHECKPOINT] All users have submitted their votes");

  // External user sends a join request
  console.log("\nExternal user requests join...");
  await requestJoinGroup(externalUser, spk_G, wasm);
}

simulateFiveClients().catch(console.error);

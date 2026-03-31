import axios from "axios";
import WebSocket from "ws";

axios.defaults.baseURL = "http://localhost:3000";
axios.defaults.withCredentials = true;

let openSockets = [];
let votes = [];
let finished = false;
let alpha = null;
let Tpp = null;

let serverPK = null; // Server public key
let thirdPartyPK = null; // Server public key

let invitedUsers = new Set();
let votedUsers = new Set();

let initial_group_size = 10;

// Read input parameter for initial_group_size
const args = process.argv.slice(2);
if (args.length >= 1 && Number(args[0])) {
  initial_group_size = Number(args[0]);
}
console.log("Running with initial_group_size=" + initial_group_size);

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

// Create a user and send public key to server
async function createClient(wasm) {
  const keys = JSON.parse(wasm.generateKeys());
  await axios.post("/send-user-key", {
    browser: false,
    u_i: keys.u_i,
    upk: keys.upk_u_i,
  });
  return { ...keys, tag: null };
}

// Create group
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

// Create election
async function createElection(election_info, upk, u_i) {
  await axios.post("/create-election", {
    browser: false,
    election_info,
    upk,
    u_i,
  });
}

// Vote in election
async function voteOnElection(wasm, election, voter) {
  const numOptions = election.options.length;
  const choice = Math.floor(Math.random() * numOptions);
  votes.push(choice);

  const ballot = JSON.parse(
    wasm.encryptVote(voter.v_i, election.epk_E, choice),
  ).ballot;

  await axios.post("/send-vote", {
    browser: false,
    u_i: voter.u_i,
    upk: voter.upk_u_i,
    epk_E: election.epk_E,
    ballot,
  });
}

// Confirm group membership
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

// Single WebSocket per user
function openWebSocketConnection(user, spk_G, wasm) {
  return new Promise((resolve, reject) => {
    const ws = new WebSocket(
      `ws://localhost:8080/server_war/ws/${user.upk_u_i}`,
    );

    ws.on("open", () => {
      console.log(`WebSocket opened for user ${user.upk_u_i.slice(0, 10)}`);
      user.websocket = ws;
      openSockets.push(ws);
      resolve();
    });

    // Attach a promise that resolves when group confirmation finishes
    user.groupConfirmedPromise = new Promise(
      (confirmResolve, confirmReject) => {
        ws.on("message", async (data) => {
          /*console.log(
            `User ${user.upk_u_i.slice(0, 10)} received:`,
            data.toString()
          );
          const msg = JSON.parse(data);*/

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

          console.log("sharedSecret");
          console.log(sharedSecret);
          console.log("msg: ");
          console.log(msg);
          console.log("hmac");
          console.log(hmac);

          const valid = wasm.crypto.verifyHmacHex(sharedSecret, msg, hmac);

          if (!valid) {
            console.error("Invalid HMAC - message rejected");
            return;
          }

          msg = payload;

          // ------------ HMAC Verification [END] -------------

          switch (msg.type) {
            case "group_invite": {
              const spk = msg.groupId;
              const id = msg.ID;
              const tag = JSON.parse(
                wasm.generateTag(user.upk_u_i, user.v_i, spk),
              ).z_i_G;
              user.tag = tag;

              const payload = { type: "ack", upk: user.upk_u_i };

              const payloadString = JSON.stringify(payload);

              // ------------ HMAC Computation [START] -------------
              const sharedSecret = wasm.crypto.deriveSharedSecretHex(
                user.u_i,
                serverPK,
              );

              const hmac = wasm.crypto.createHmacHex(
                sharedSecret,
                payloadString,
              );
              // ------------ HMAC Computation [END] -------------

              const envelope = {
                hmac,
                payload, // object form for convenience
              };

              ws.send(JSON.stringify(envelope));

              console.log(
                `User ${user.upk_u_i.slice(0, 10)} acknowledged group invite`,
              );

              try {
                await retryConfirmGroup(spk_G, tag, user.upk_u_i, user.u_i, id);
                console.log(
                  `User ${user.upk_u_i.slice(
                    0,
                    10,
                  )} confirmed group successfully`,
                );
                confirmResolve(); // signal this user is confirmed
              } catch (err) {
                console.error("Confirm failed:", err.message);
                confirmReject(err);
              }
              break;
            }

            case "election_invite": {
              console.log(
                `User ${user.upk_u_i.slice(0, 10)} received election invite`,
              );
              invitedUsers.add(user.upk_u_i);
              await voteOnElection(wasm, msg, user);
              votedUsers.add(user.upk_u_i);
              console.log(`User ${user.upk_u_i.slice(0, 10)} cast vote`);
              break;
            }

            case "int_step_1":
            case "int_step_1_C": {
              const result = JSON.parse(
                wasm.groupInitExp(msg.groupSize, msg.tagList),
              );
              alpha = result.alpha;

              const payload = {
                type:
                  msg.type === "int_step_1"
                    ? "int_step_1_reply"
                    : "int_step_1_reply_C",
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

              const hmac = wasm.crypto.createHmacHex(
                sharedSecret,
                payloadString,
              );
              // ------------ HMAC Computation [END] -------------

              const envelope = {
                hmac,
                payload, // object form for convenience
              };

              try {
                ws.send(JSON.stringify(envelope));
                console.log("Sent " + JSON.stringify(envelope));
              } catch (err) {
                console.error("Error sending int_step_1_reply:", err);
              }
              break;
            }

            case "int_step_2": {
              const result = JSON.parse(
                wasm.shuffleUser(user.u_i, user.upk_u_i, msg.Tprime),
              );
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

              const hmac = wasm.crypto.createHmacHex(
                sharedSecret,
                payloadString,
              );
              // ------------ HMAC Computation [END] -------------

              const envelope = {
                hmac,
                payload, // object form for convenience
              };

              ws.send(JSON.stringify(envelope));
              break;
            }

            case "int_step_3":
            case "int_step_3_C": {
              const result = JSON.parse(
                wasm.intersection(
                  msg.groupSize,
                  alpha,
                  msg.Ws,
                  msg.overline_spk_i_start_G,
                  msg.Tpp || Tpp,
                  msg.minVote,
                  msg.maxVote,
                  msg.nBallots,
                ),
              );

              const payload = {
                type:
                  msg.type === "int_step_3"
                    ? "int_step_3_reply"
                    : "int_step_3_reply_C",
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

              const hmac = wasm.crypto.createHmacHex(
                sharedSecret,
                payloadString,
              );
              // ------------ HMAC Computation [END] -------------

              const envelope = {
                hmac,
                payload, // object form for convenience
              };

              ws.send(JSON.stringify(envelope));

              if (msg.type === "int_step_3_C") {
                const valuesArray = result.X.trim().split(" ").map(Number);
                console.log("Result of intersection:", valuesArray.sort());
                console.log("Original votes:", votes.sort());

                console.log("=== ELECTION SUMMARY ===");
                console.log(
                  `Invited: ${invitedUsers.size}/${initial_group_size}`,
                );
                console.log(`Voted: ${votedUsers.size}/${initial_group_size}`);

                if (invitedUsers.size !== initial_group_size)
                  console.warn(
                    "Some users did NOT receive the election invite!",
                  );
                if (votedUsers.size !== initial_group_size)
                  console.warn("Some users did NOT vote!");

                openSockets.forEach(
                  (s) => s.readyState === WebSocket.OPEN && s.close(),
                );
                finished = true;
                process.exit(0);
              }
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

              const hmac = wasm.crypto.createHmacHex(
                sharedSecret,
                payloadString,
              );
              // ------------ HMAC Computation [END] -------------

              const envelope = {
                hmac,
                payload, 
              };

              ws.send(JSON.stringify(envelope));
              break;
            }

            case "int_step_5": {
              console.log(msg.message);
              openSockets.forEach(
                (s) => s.readyState === WebSocket.OPEN && s.close(),
              );
              finished = true;
              process.exit(0);
              break;
            }

            default:
              console.warn("Unhandled WS msg:", msg.type);
          }
        });
      },
    );

    ws.on("error", (err) => {
      console.error(`WebSocket error for ${user.upk_u_i.slice(0, 10)}:`, err);
      reject(err);
    });

    ws.on("close", (code, reason) => {
      console.log(
        `WebSocket closed for user ${user.upk_u_i.slice(
          0,
          10,
        )}. Code: ${code}, Reason: ${reason}`,
      );
    });
  });
}

// Helpers
function getScore(scores) {
  const sum = scores.reduce((acc, val) => acc + val, 0);
  return sum / scores.length;
}

function getCount(votes) {
  const counts = {};
  for (const v of votes) counts[v] = (counts[v] || 0) + 1;
  return Object.keys(counts)
    .map(Number)
    .sort((a, b) => a - b)
    .map((k) => `${k}: ${counts[k]} vote${counts[k] > 1 ? "s" : ""}`)
    .join("; ");
}

// ---------------------------- SIMULATION -----------------------------------
async function simulateClients() {
  const wasm = await loadWasmFunctions();
  await loadKeys();

  console.log(`Creating ${initial_group_size} clients...`);
  const clients = await Promise.all(
    Array.from({ length: initial_group_size }, () => createClient(wasm)),
  );
  console.log("[CHECKPOINT] All clients created");

  const creator = clients[0];
  const { spk_G } = await createGroup(creator, clients);
  console.log("[CHECKPOINT] Created group with ID: " + spk_G);

  // Open one persistent WebSocket per user
  console.log("Connecting via WebSocket...");
  await Promise.all(
    clients.map((c) => openWebSocketConnection(c, spk_G, wasm)),
  );
  console.log("[CHECKPOINT] All users connected via WebSocket");

  // Wait until all users have confirmed their group invite
  console.log("Waiting for all users to confirm group invite...");
  await Promise.all(
    clients.map((c) => c.groupConfirmedPromise), 
  );
  console.log("[CHECKPOINT] All users confirmed the group invitation");

  // Create election
  const deadline = new Date(Date.now() + 0.2 * 60 * 1000); // 1min from now
  const election_info = {
    name: "Approve Internal Regulations Updates?",
    options: ["Yes", "No", "Blank"],
    deadline,
    group_spk: spk_G,
  };

  // Create the election
  await createElection(election_info, creator.upk_u_i, creator.u_i);
  console.log("[CHECKPOINT] Election created");

  // Block until finished
  await new Promise((resolve) => {
    const check = setInterval(() => {
      if (finished) {
        clearInterval(check);
        resolve();
      }
    }, 1000);
  });

  console.log(
    "Finished with " +
      votes.length +
      " votes for " +
      initial_group_size +
      " users",
  );
}

simulateClients().catch((err) => {
  console.error(err);
  process.exit(1);
});

// simulateElection.js
import axios from "axios";
import WebSocket from "ws";
import { performance } from "perf_hooks";

axios.defaults.baseURL = "http://localhost:3000";
axios.defaults.withCredentials = true;

let openSockets = [];
let votes = [];
let finished = false;
let alpha = null;
let Tpp = null;

let invitedUsers = new Set();
let votedUsers = new Set();

let serverPK = null; // Server public key
let thirdPartyPK = null; // Server public key

let initial_group_size = 10;

// Metrics state
let metrics = {
  startTime: performance.now(),
  voteEncryption: new Map(), // upk -> ms
  intersectionStart: null,
  intersectionEnd: null,
  groupCreationStart: null,
  groupCreationEnd: null,
  bandwidth: new Map(), // upk -> { sent, received }
};

// ---------------- CLI ----------------
const args = process.argv.slice(2);
if (args.length >= 1 && Number(args[0])) {
  initial_group_size = Number(args[0]);
}
console.log("Running with initial_group_size=" + initial_group_size);

// ---------------- METRICS REPORT ----------------
function printMetrics() {
  const runtime = performance.now() - metrics.startTime;
  const mem = process.memoryUsage();

  console.log("\n========= METRICS =========");
  console.log("Runtime (ms):", runtime.toFixed(2));
  console.log("RAM usage (MB):", (mem.rss / 1024 / 1024).toFixed(2));

  // vote encryption
  if (metrics.voteEncryption.size) {
    const values = Array.from(metrics.voteEncryption.values());
    const avg = values.reduce((a, b) => a + b, 0) / values.length;
    console.log("Vote encryption avg (ms):", avg.toFixed(2));
  }

  if (metrics.intersectionStart && metrics.intersectionEnd) {
    console.log(
      "Intersection latency (ms):",
      (metrics.intersectionEnd - metrics.intersectionStart).toFixed(2),
    );
  }

  if (metrics.groupCreationStart && metrics.groupCreationEnd) {
    console.log(
      "Group creation latency (ms):",
      (metrics.groupCreationEnd - metrics.groupCreationStart).toFixed(2),
    );
  }

  // bandwidth
  for (let [upk, bw] of metrics.bandwidth.entries()) {
    console.log(
      `User ${upk.slice(0, 8)}... | Sent: ${bw.sent}B | Received: ${
        bw.received
      }B`,
    );
  }
  console.log("==========================\n");
}

// ---------------- REST + WASM ----------------
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

async function createClient(wasm) {
  const keys = JSON.parse(wasm.generateKeys());
  await axios.post("/send-user-key", {
    browser: false,
    u_i: keys.u_i,
    upk: keys.upk_u_i,
  });
  metrics.bandwidth.set(keys.upk_u_i, { sent: 0, received: 0 });
  return { ...keys, tag: null };
}

async function createGroup(creator, members) {
  const upks = members.map((c) => c.upk_u_i);
  metrics.groupCreationStart = performance.now();
  const res = await axios.post("/request-group-creation", {
    browser: false,
    name: "Some name",
    u_i: creator.u_i,
    upk: creator.upk_u_i,
    members: upks,
  });
  metrics.groupCreationEnd = performance.now();
  return { spk_G: res.data.spk_G, s_G: res.data.s_G };
}

async function createElection(election_info, upk, u_i) {
  await axios.post("/create-election", {
    browser: false,
    election_info,
    upk,
    u_i,
  });
}

async function voteOnElection(wasm, election, voter) {
  const numOptions = election.options.length;
  const choice = Math.floor(Math.random() * numOptions);
  votes.push(choice);

  const start = performance.now();
  const ballot = JSON.parse(
    wasm.encryptVote(voter.v_i, election.epk_E, choice),
  ).ballot;
  const end = performance.now();

  metrics.voteEncryption.set(voter.upk_u_i, end - start);

  await axios.post("/send-vote", {
    browser: false,
    u_i: voter.u_i,
    upk: voter.upk_u_i,
    epk_E: election.epk_E,
    ballot,
  });
}

// ---------------- SOCKET HANDLER (modified) ----------------
function openWebSocketConnection(user, spk_G, wasm) {
  return new Promise((resolve, reject) => {
    const ws = new WebSocket(
      `ws://localhost:8080/server_war/ws/${user.upk_u_i}`,
    );

    // wrap send to count bandwidth
    const origSend = ws.send;
    ws.send = function (data) {
      metrics.bandwidth.get(user.upk_u_i).sent += Buffer.byteLength(data);
      origSend.call(ws, data);
    };

    ws.on("open", () => {
      user.websocket = ws;
      openSockets.push(ws);
      resolve();
    });

    user.groupConfirmedPromise = new Promise((confirmResolve) => {
      ws.on("message", async (data) => {
        metrics.bandwidth.get(user.upk_u_i).received += Buffer.byteLength(data);
        //const msg = JSON.parse(data);

        const fullMsg = JSON.parse(data);
        /*console.log(
          `User ${user.upk_u_i.slice(0, 10)} received message:`,
          fullMsg,
        );*/

        // ------------ HMAC Verification [START] -------------
        const { hmac, payload } = fullMsg;
        // Get upk of the sender (Tomcat server)

        const sharedSecret = wasm.crypto.deriveSharedSecretHex(
          user.u_i,
          serverPK,
        );

        //console.log("Printing shared secret: " + sharedSecret);

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
          case "group_invite": {
            const spk = msg.groupId;
            const id = msg.ID;
            const tag = JSON.parse(
              wasm.generateTag(user.upk_u_i, user.v_i, spk),
            ).z_i_G;
            user.tag = tag;

            const payload = { type: "ack", upk: user.upk_u_i, id };

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

            //ws.send(JSON.stringify({ type: "ack", upk: user.upk_u_i, id }));
            await axios.post("/confirm-group", {
              browser: false,
              spk,
              tag,
              upk: user.upk_u_i,
              u_i: user.u_i,
              id,
            });
            confirmResolve();
            break;
          }

          case "election_invite": {
            invitedUsers.add(user.upk_u_i);
            await voteOnElection(wasm, msg, user);
            votedUsers.add(user.upk_u_i);

            // Check if all voted
            if (votedUsers.size === initial_group_size) {
              console.log("[CHECKPOINT] All users have voted");
            }
            break;
          }

          case "int_step_1":
          case "int_step_1_C": {
            console.log("Intersection has started");
            if (!metrics.intersectionStart) {
              metrics.intersectionStart = performance.now();
            }
            //metrics.intersectionStart = performance.now();
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

            const hmac = wasm.crypto.createHmacHex(sharedSecret, payloadString);
            // ------------ HMAC Computation [END] -------------

            const envelope = {
              hmac,
              payload, // object form for convenience
            };

            try {
              ws.send(JSON.stringify(envelope));
              
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

            const hmac = wasm.crypto.createHmacHex(sharedSecret, payloadString);
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

            metrics.intersectionEnd = performance.now();

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

            const hmac = wasm.crypto.createHmacHex(sharedSecret, payloadString);
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
              printMetrics();
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

            const hmac = wasm.crypto.createHmacHex(sharedSecret, payloadString);
            // ------------ HMAC Computation [END] -------------

            const envelope = {
              hmac,
              payload, // object
            };

            ws.send(JSON.stringify(envelope));
            break;
          }

          case "int_step_5": {
            //console.log(msg.message);
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
    });
  });
}

// ---------------- SIMULATION ----------------
async function simulateClients() {
  const wasm = await loadWasmFunctions();
  loadKeys();

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

  const deadline = new Date(Date.now() + 0.2 * 60 * 1000);
  const election_info = {
    name: "Approve Internal Regulations Updates?",
    options: ["Yes", "No", "Blank"],
    deadline,
    group_spk: spk_G,
  };

  await createElection(election_info, creator.upk_u_i, creator.u_i);
  console.log("[CHECKPOINT] Election created");

  await new Promise((resolve) => {
    const check = setInterval(() => {
      if (finished) {
        clearInterval(check);
        resolve();
      }
    }, 1000);
  });
}

simulateClients().catch((err) => {
  console.error(err);
  process.exit(1);
});

import axios from "axios";
import WebSocket from "ws";

axios.defaults.baseURL = "http://localhost:3000";
axios.defaults.withCredentials = true;

let openSockets = [];
let votes = [];
let finished = false;
let alpha = null;
let Tpp = null;

let initial_group_size = 50;
let n_voting_users = 40;

// Read CLI arguments
const args = process.argv.slice(2);
if (args.length >= 2 && Number(args[0]) && Number(args[1])) {
  initial_group_size = Number(args[0]);
  n_voting_users = Number(args[1]);
}
console.log(
  "Running with initial_group_size=" +
    initial_group_size +
    " and n_voting_users=" +
    n_voting_users,
);

// ---------------- WASM LOADER ----------------
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

// ---------------- HELPERS ----------------
async function createClient(wasm) {
  const keys = JSON.parse(wasm.generateKeys());
  await axios.post("/send-user-key", {
    browser: false,
    u_i: keys.u_i,
    upk: keys.upk_u_i,
  });
  return { ...keys, tag: null, websocket: null };
}

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
        `Confirmed group ${spk.slice(0, 10)} for ${upk.slice(0, 10)}`,
      );
      return;
    } catch (err) {
      await new Promise((res) => setTimeout(res, delayMs));
    }
  }
  throw new Error(`Failed to confirm group for user ${upk.slice(0, 10)}`);
}

function pickRandomSubset(array, fraction) {
  const shuffled = array.slice().sort(() => Math.random() - 0.5);
  const count = Math.floor(array.length * fraction);
  return shuffled.slice(0, count);
}

function getScore(scores) {
  const sum = scores.reduce((acc, val) => acc + val, 0);
  return sum / scores.length;
}

// ---------------- STATE ----------------
let confirmedUsers = new Set();
let groupSizeExpected = 0;
let externalUserRef = null;
let wasmRef = null;
let spkRef = null;

// ---------------- WEBSOCKET HANDLER ----------------
function openWebSocketConnection(user, wasm) {
  return new Promise((resolve, reject) => {
    const ws = new WebSocket(
      `ws://localhost:8080/server_war/ws/${user.upk_u_i}`,
    );
    user.websocket = ws;
    openSockets.push(ws);

    ws.on("open", () => {
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

      console.log("Reached switch");

      switch (msg.type) {
        case "group_invite": {
          const spk = msg.groupId;
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

          const hmac = wasm.crypto.createHmacHex(sharedSecret, payloadString);
          // ------------ HMAC Computation [END] -------------

          const envelope = {
            hmac,
            payload, // object form for convenience
          };

          ws.send(JSON.stringify(envelope));

          //ws.send(JSON.stringify({ type: "ack", upk: user.upk_u_i }));
          await retryConfirmGroup(spk, tag, user.upk_u_i, user.u_i, msg.ID);

          confirmedUsers.add(user.upk_u_i);

          if (confirmedUsers.size === groupSizeExpected && externalUserRef) {
            console.log("[CHECKPOINT] All group members confirmed");
            await requestJoinGroup(externalUserRef, spkRef, wasmRef);
            console.log("[CHECKPOINT] External user requested join");
          }
          break;
        }
        case "int_step_1": {
          const result = JSON.parse(
            wasm.groupInitExp(msg.groupSize, msg.tagList),
          );
          alpha = result.alpha;
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
            payload, // object
          };

          ws.send(JSON.stringify(envelope));
          console.log("Sent " + JSON.stringify(envelope));

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

          console.log("Printing result: ");
          console.log(result);

          const valuesArray = result.X.trim().split(" ").map(Number);
          console.log("Result of intersection:", valuesArray.sort());
          console.log("Original votes:", votes.sort());

          console.log(
            "Expected avg: " +
              getScore(votes) +
              " | Produced avg: " +
              getScore(valuesArray),
          );

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
          openSockets.forEach((socket) => {
            if (socket.readyState === WebSocket.OPEN) socket.close();
          });
          console.log("[CHECKPOINT] Simulation complete!");
          finished = true;
          process.exit(0);
          break;
        }
      }
    });

    ws.on("error", (err) => {
      reject(err);
    });
  });
}

// ---------------- SIMULATION ----------------
async function simulate() {
  const wasm = await loadWasmFunctions();
  wasmRef = wasm;

  loadKeys();

  const clients = await Promise.all(
    Array.from({ length: n_voting_users }, () => createClient(wasm)),
  );

  const externalUser = clients[0];
  externalUserRef = externalUser;

  const groupMembers = clients.slice(1, 1 + initial_group_size);
  const creator = clients[1];

  const { spk_G } = await createGroup(creator, groupMembers);
  spkRef = spk_G;
  groupSizeExpected = groupMembers.length;

  await Promise.all(clients.map((c) => openWebSocketConnection(c, wasm)));

  const eligibleVoters = clients.filter((c) => c !== externalUser);
  await Promise.all(
    eligibleVoters.map((voter) =>
      voteForUser(voter, externalUser.upk_u_i, wasm, groupMembers),
    ),
  );

  console.log("[CHECKPOINT] Votes submitted, waiting for confirmations...");
}

simulate().catch(console.error);

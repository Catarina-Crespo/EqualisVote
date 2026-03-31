const express = require("express");
const axios = require("axios");
const WebSocket = require("ws");
const path = require("path");
var cors = require("cors");

// ----------------------- CONSTANTS -----------------------

const port = 5000; // Port where the elections server will run
const tomcatUrl = "http://localhost:8080/server_war";
const tomcatWS = "ws://localhost:8080/server_war/ws/party";

const INT_STEP_2 = "int_step_2";
const INT_STEP_2_REPLY_C = "int_step_2_reply_C";

// ----------------------- VARIABLES -----------------------

let dataStore = {}; // In-memory storage, just for testing (in real-life, it would need some DB (mongoDB or Redis) or store this to a file)

let keys = null;
let serverSharedSecret = null;
let serverPK = null;

const app = express();
app.use(
  express.json({
    verify: (req, res, buf) => {
      req.rawBody = buf;
    },
  }),
); // Middleware to parse JSON requests

app.use(
  cors({
    origin: `http://localhost:${port}`,
    credentials: true,
  }),
);

// ----------------------- WASM LOADER -----------------------

async function loadWasmFunctions() {
  const createElectionMod = require("./binaries/create_election.js");
  const shuffleMod = require("./binaries/shuffle_user.js");
  const cryptoMod = require("./binaries/crypto.js").default;

  const [election, shuffle, crypto] = await Promise.all([
    createElectionMod(),
    shuffleMod.default(),
    cryptoMod(),
  ]);

  return {
    generateElection: election.cwrap("create_election", "string", [
      "string",
      "string",
      "string",
    ]),
    shuffleUser: shuffle.cwrap("shuffle_user", "string", [
      "string",
      "string",
      "string",
    ]),
    // ---- Embind-based crypto module ----
    crypto: {
      deriveSharedSecretHex: crypto.derive_shared_secret_hex,
      createHmacHex: crypto.create_hmac_hex,
      verifyHmacHex: crypto.verify_hmac_hex,
    },
  };
}

// ----------------------- PERSISTENT WS -----------------------

function createPersistentWebSocket(url, handlers, retryDelay = 1000) {
  let ws;

  function connect() {
    console.log(`Connecting to WebSocket ${url}...`);
    ws = new WebSocket(url, {
      maxPayload: 50 * 1024 * 1024, // increased to 50 MB
    });

    ws.on("open", () => {
      console.log("Connected to WebSocket");
      retryDelay = 1000;            // reset delay after successful connect
      if (handlers.onopen) handlers.onopen(ws);
    });

    ws.on("message", (event) => {
      if (handlers.onmessage) handlers.onmessage(event, ws);
    });

    ws.on("close", () => {
      console.warn("WebSocket closed. Reconnecting...");
      if (handlers.onclose) handlers.onclose();
      setTimeout(connect, retryDelay);
      retryDelay = Math.min(retryDelay * 2, 30000); // exponential backoff up to 30s
    });

    ws.on("error", (err) => {
      console.error("WebSocket error:", err.message);
      ws.close();     // trigger reconnect via onclose
    });
  }

  connect();
  return () => ws;    // return accessor for current ws instance
}

// ----- Function to retrieve the server public key
async function initServerKey() {
  const res = await axios.get(`${tomcatUrl}/key`);
  serverPK = res.data.pk;
  console.log("Server public key loaded");
}

// ----------------------- SERVER START -----------------------

async function start() {
  const wasm = await loadWasmFunctions();
  await initServerKey();

  // Create own public and private keys
  const { e_E, epk_E, epk_proof } = JSON.parse(await wasm.generateElection());
  keys = { pk: epk_E, sk: e_E };

  //console.log("sk:", keys.sk, typeof keys.sk);
  //console.log("s pk:", serverPK, typeof serverPK);

  serverSharedSecret = wasm.crypto.deriveSharedSecretHex(keys.sk, serverPK);

  // ----------------------- ENDPOINTS -----------------------

  // POST endpoint to create an election
  app.post("/create-election", async (req, res) => {
    // ------------ HMAC Verification [START] -------------
    const queryUpk = req.query.upk;
    const macHex = req.headers["x-mac"];

    // HMAC Content
    const rawBody = req.rawBody.toString("utf8");

    //console.log("keys.sk: " + keys.sk);
    //console.log("upk: " + queryUpk);

    let tmpSharedSecret = wasm.crypto.deriveSharedSecretHex(keys.sk, queryUpk);
    //console.log("Shared secret: " + tmpSharedSecret);
    let valid = wasm.crypto.verifyHmacHex(tmpSharedSecret, rawBody, macHex);

    if (!valid) {
      return res
        .status(401)
        .json({ message: "HMAC was invalid. The election was not created." });
    }
    // ------------ HMAC Verification [END] -------------

    const { name, options, deadline, group_spk } = req.body;
    console.log("Parsed body:");
    console.log(name, options, deadline, group_spk);

    // Generate election keys by calling the wasm module
    const { e_E, epk_E, epk_proof } = JSON.parse(wasm.generateElection());

    // Store election public key with the associated secret
    dataStore[epk_E] = e_E;

    const body = {
      epk_E,
      name,
      options,
      deadline,
      group_spk,
    };

    // ------------ HMAC Computation [START] -------------

    // HMAC content
    const msg = JSON.stringify(body);

    // HMAC Computation
    const mac2 = wasm.crypto.createHmacHex(serverSharedSecret, msg);
    // ------------ HMAC Computation [END] -------------

    // Send message to the Tomcat server with the election information (to be forwarded to the group members)
    try {
      console.log("Sending request to Tomcat server (via axios)");
      const response = await axios.post(`${tomcatUrl}/createElection`, body, {
        headers: {
          "X-Message": msg,
          "X-MAC": mac2,
        },
      });
      return res.json(response.data);
    } catch (error) {
      console.error("Error sending the message to Tomcat:", error.message);
      console.error("Error sending the message to Tomcat:", error.message);
      return res
        .status(error.response?.status || 500)
        .json(error.response?.data || { message: "Tomcat error" });
    }
  });

  // GET endpoint to get the third-party's public key
  app.get("/key", async (req, res) => {
    res.status(200).json({ pk: keys.pk });
  });

  // ----------------------- WEBSOCKET HANDLER -----------------------

  const getWs = createPersistentWebSocket(tomcatWS, {
    onopen: () => {
      console.log("Connected to Tomcat server WebSocket");
    },
    onmessage: async (event, ws) => {
      const fullMsg = JSON.parse(event);
      /*console.log(
        `Received message:`,
        fullMsg,
      );*/

      // ------------ HMAC Verification [START] -------------
      const { hmac, payload } = fullMsg;

      // Verify
      let msg = JSON.stringify(payload);

      /*console.log("sharedSecret");
      console.log(serverSharedSecret);
      console.log("msg: ");
      console.log(msg);
      console.log("hmac");
      console.log(hmac);*/

      const valid = wasm.crypto.verifyHmacHex(serverSharedSecret, msg, hmac);

      if (!valid) {
        console.error("Invalid HMAC - message rejected");
        return;
      }

      msg = payload;

      // ------------ HMAC Verification [END] -------------

      if (msg.type === INT_STEP_2) {
        // Generate Tpp by calling the wasm module
        const result = JSON.parse(
          wasm.shuffleUser(dataStore[msg.upk], msg.upk, msg.Tprime),
        );

        const payload = {
          type: INT_STEP_2_REPLY_C,
          upk: msg.upk,
          spk: msg.spk,
          group_size: msg.groupSize,
          Tpp: result.Tpp,
        };

        const payloadString = JSON.stringify(payload);

        // ------------ HMAC Computation [START] -------------
        const hmac = wasm.crypto.createHmacHex(serverSharedSecret, payloadString);
        // ------------ HMAC Computation [END] -------------

        const envelope = {
          hmac,
          payload, // object form for convenience
        };

        ws.send(JSON.stringify(envelope));

      }
    },
    onclose: () => {
      console.log("Tomcat WebSocket closed, will retry...");
    },
  });

  // ----------------------- EXPRESS APP -----------------------

  app.listen(port, () => {
    console.log(`Client app running at http://localhost:${port}`);
  });
}

start();

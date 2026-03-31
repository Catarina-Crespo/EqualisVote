import express from "express";
import axios from "axios";
import path from "path";
import cors from "cors";
import { fileURLToPath } from "url";
import session from "express-session";
import {
  loadWasmFunctions,
  deriveSharedSecret,
  createHmac,
} from "./src/backend/wasmAPI.js";

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

const app = express();
let port = 3000; // default
const tomcatUrl = "http://localhost:8080/server_war";
const thirdPartyURL = "http://localhost:5000";

// Keys
let serverPK = null; // Server public key
let thirdPartyPK = null; // Server public key

const args = process.argv.slice(2);
//console.log(args)
if (args.length == 0 || !Number(args[0]))
  console.log(
    "Arguments passed do not correspond to the specification. Using default port",
  );
else port = Number(args[0]);

app.use(express.json()); // Middleware to parse JSON requests
app.use(express.static("public")); // Serve the frontend from the 'public' folder

app.use(
  cors({
    origin: [`http://localhost:${port}`, "http://localhost:5173"],
    credentials: true,
  }),
);

app.use(
  session({
    name: "middleware.sid",
    secret: "very-secret-string",
    resave: false,
    saveUninitialized: false,
    cookie: { httpOnly: true },
  }),
);

// ----- Function to retrieve the server public key
async function initServerKey() {
  const res = await axios.get(`${tomcatUrl}/key`);
  serverPK = res.data.pk;
  console.log("Server public key loaded");
}

// ----- Function to retrieve third party's public key
async function initThirdPartyKey() {
  const res = await axios.get(`${thirdPartyURL}/key`);
  thirdPartyPK = res.data.pk;
  console.log("Third party public key loaded");
}

initServerKey();
initThirdPartyKey();
const wasm = await loadWasmFunctions();

app.get("/keys", async (req, res) => {
  res.status(200).json({ thirdPartyPK, serverPK });
});

// Local function to encrypt a vote
app.post("/encrypt-vote", async (req, res) => {
  let { browser, votee_upk, value } = req.body;

  if (browser) {
    const ballot = JSON.parse(
      wasm.encryptVote(req.session.userKeys.v_i, votee_upk, value),
    ).ballot;

    res.status(200).json({ ballot });
  }
});

// GET request to the /hello endpoint
app.get("/fetch-data", async (req, res) => {
  try {
    const cookieHeader = req.headers.cookie;

    const response = await axios.get(`${tomcatUrl}/hello`, {
      headers: {
        Cookie: cookieHeader,
      },
      withCredentials: true,
    });

    console.log("Client sent cookies:", req.headers.cookie);

    res.json(response.data); // Send the response data back to the frontend
  } catch (error) {
    console.error("Error fetching data:", error);
    res.status(500).json({ message: "Error fetching data from Tomcat server" });
  }
});

// POST request to send user key to the server and create a user using the /createUser endpoint
app.post("/send-user-key", async (req, res) => {
  let { browser, upk, u_i } = req.body;
  let sharedSecret = null;
  let v_i = null;

  try {
    // If browser, generate keys in middleware
    if (browser) {
      const genKeys = JSON.parse(wasm.generateKeys());

      // store secrets server-side ONLY
      req.session.userKeys = {
        u_i: genKeys.u_i,
        upk: genKeys.upk_u_i,
        v_i: genKeys.v_i,
      };

      upk = genKeys.upk_u_i;
      u_i = genKeys.u_i;
      v_i = genKeys.v_i;

      // ------------ HMAC Computation [START] -------------
      req.session.sharedSecret = await deriveSharedSecret(
        wasm,
        genKeys.u_i,
        serverPK,
      );
      sharedSecret = req.session.sharedSecret;
      console.log("Printing keys: ");
      console.log(req.session.userKeys);
      console.log("Printing upk: " + upk);
      console.log("Printing u_i: " + genKeys.u_i);
      console.log("Printing server key: " + serverPK);
    } else {
      sharedSecret = await deriveSharedSecret(wasm, u_i, serverPK);
    }

    const msg = `upk=${encodeURIComponent(upk)}`;
    console.log("sharedSecretHex:", sharedSecret, typeof sharedSecret);
    console.log("message:", msg, typeof msg);

    const mac = createHmac(wasm, sharedSecret, msg);
    // ------------ HMAC Computation [END] -------------

    const response = await axios.post(
      `${tomcatUrl}/createUser?upk=${encodeURIComponent(upk)}`,
      null,
      {
        headers: {
          "X-Message": msg,
          "X-MAC": mac,
        },
        withCredentials: true,
      },
    );

    // Extract Set-Cookie from Tomcat response and forward it
    const setCookieHeader = response.headers["set-cookie"];
    if (setCookieHeader) {
      res.setHeader("Set-Cookie", setCookieHeader);
    }

    res.json({ upk, u_i, v_i });
  } catch (error) {
    console.error("Error forwarding user key to Tomcat:", error);
    res.status(500).json({ message: "Failed to send user key to Tomcat" });
  }
});

// POST request to send user key to the server and create a user using the /createUser endpoint
app.post("/save-user-key", async (req, res) => {
  const { upk, u_i, v_i } = req.body;

  // store secrets server-side ONLY
  req.session.userKeys = {
    u_i: u_i,
    upk: upk,
    v_i: v_i,
  };

  console.log("Saved keys:");
  console.log(req.session.userKeys);

  res.status(200).json({ message: "Keys saved on the middleware" });
});

// POST request to confirm a group
app.post("/confirm-group", async (req, res) => {
  let { browser, spk, tag, upk, u_i, id } = req.body;
  let sharedSecret = null;

  try {
    // ------------ HMAC Computation [START] -------------
    if (!browser) sharedSecret = await deriveSharedSecret(wasm, u_i, serverPK);
    else {
      if (tag == "yes")
        tag = JSON.parse(
          await wasm.generateTag(
            req.session.userKeys.upk,
            req.session.userKeys.v_i,
            spk,
          ),
        ).z_i_G;
      else tag = null;
      sharedSecret = req.session.sharedSecret;
    }

    // HMAC content
    const msg = `spk=${encodeURIComponent(
      spk,
    )}&tag=${encodeURIComponent(tag)}&upk=${encodeURIComponent(
      upk,
    )}&id=${encodeURIComponent(id)}`;

    // HMAC Computation
    const mac = createHmac(wasm, sharedSecret, msg);

    // ------------ HMAC Computation [END] -------------

    // Tomcat request
    const response = await axios.post(
      `${tomcatUrl}/confirm?spk=${encodeURIComponent(
        spk,
      )}&tag=${encodeURIComponent(tag)}&upk=${encodeURIComponent(
        upk,
      )}&id=${encodeURIComponent(id)}`,
      null,
      {
        headers: {
          "X-Message": msg,
          "X-MAC": mac,
        },
        withCredentials: true,
      },
    );
    res.json(response.data);
  } catch (error) {
    console.error("Error forwarding user-group tag to Tomcat:", error);
    res
      .status(500)
      .json({ message: "Failed to send user-group tag to Tomcat" });
  }
});

// POST request to vote on a user (or election)
app.post("/send-vote", async (req, res) => {
  let { browser, votee, epk_E, ballot, u_i, upk } = req.body;
  let sharedSecret = null;

  console.log("Logging votee:");
  console.log(votee);
  console.log("Logging epk:");
  console.log(epk_E);

  try {
    // ------------ HMAC Computation [START] -------------
    if (!browser) sharedSecret = await deriveSharedSecret(wasm, u_i, serverPK);
    else {
      sharedSecret = req.session.sharedSecret;
      upk = req.session.userKeys.upk;
    }

    if (typeof votee === "undefined") {
      // HMAC content
      const msg = `upk=${encodeURIComponent(
        upk,
      )}&epk=${encodeURIComponent(epk_E)}&ballot=${encodeURIComponent(ballot)}`;

      // HMAC Computation
      const mac = createHmac(wasm, sharedSecret, msg);
      // ------------ HMAC Computation [END] -------------

      const response = await axios.post(
        `${tomcatUrl}/vote?upk=${encodeURIComponent(
          upk,
        )}&epk=${encodeURIComponent(
          epk_E,
        )}&ballot=${encodeURIComponent(ballot)}`,
        null,
        {
          headers: {
            "X-Message": msg,
            "X-MAC": mac,
          },
          withCredentials: true,
        },
      );
      console.log("Sent epk: " + epk_E);
      res.json(response.data);
    } else if (typeof epk_E === "undefined") {
      // HMAC content
      const msg = `upk=${encodeURIComponent(
        upk,
      )}&votee=${encodeURIComponent(votee)}&ballot=${encodeURIComponent(ballot)}`;

      // HMAC Computation
      const mac = createHmac(wasm, sharedSecret, msg);
      // ------------ HMAC Computation [END] -------------

      const response = await axios.post(
        `${tomcatUrl}/vote?upk=${encodeURIComponent(
          upk,
        )}&votee=${encodeURIComponent(
          votee,
        )}&ballot=${encodeURIComponent(ballot)}`,
        null,
        {
          headers: {
            "X-Message": msg,
            "X-MAC": mac,
          },
          withCredentials: true,
        },
      );
      console.log("Sent votee: " + votee);
      res.json(response.data);
    } else console.log("Some error ocurred");
  } catch (error) {
    console.error("Error forwarding the vote to Tomcat:", error);
    res.status(500).json({ message: "Failed to send the vote to Tomcat" });
  }
});

// ----------------------- New implementation -----------------------

// POST to generate a group
app.post("/request-group-creation", async (req, res) => {
  let { browser, name, members, upk, u_i } = req.body; // upks must be a list separated by blank spaces

  console.log("Logging request received in middleware");
  //console.log(req.body);
  let sharedSecret = null;

  try {
    // ------------ HMAC Computation [START] -------------
    if (!browser) sharedSecret = await deriveSharedSecret(wasm, u_i, serverPK);
    else {
      sharedSecret = req.session.sharedSecret;
      upk = req.session.userKeys.upk;
    }

    // HMAC content
    const bodyObj = {
      upk,
      name,
      members,
    };
    const msg = JSON.stringify(bodyObj);
    //console.log("Sent payload: ");
    //console.log(msg);

    // HMAC Computation
    const mac = createHmac(wasm, sharedSecret, msg);
    // ------------ HMAC Computation [END] -------------

    const response = await axios.post(
      `${tomcatUrl}/createGroup?upk=${encodeURIComponent(upk)}`,
      msg,
      {
        headers: {
          "X-MAC": mac,
          "Content-Type": "application/json",
        },
        withCredentials: true,
      },
    );
    //console.log("Logging response");
    //console.log(response);
    res.json(response.data);
    //res.send(response.data);
  } catch (error) {
    console.error("Error send the upks to Tomcat:", error);
    res
      .status(500)
      .json({ message: "Failed to send the upks to Tomcat", error });
  }
});

// POST request to join a group (sent to /joinGroup endpoint)
app.post("/join-group", async (req, res) => {
  let { browser, tag, spk_G, upk, u_i } = req.body;
  let sharedSecret = null;

  try {
    // ------------ HMAC Computation [START] -------------
    if (!browser) sharedSecret = await deriveSharedSecret(wasm, u_i, serverPK);
    else {
      tag = JSON.parse(wasm.generateTag(req.session.userKeys.upk, req.session.userKeys.v_i, spk_G)).z_i_G;
      sharedSecret = req.session.sharedSecret;
      upk = req.session.userKeys.upk;
    }

    // HMAC content
    const msg = `tag=${encodeURIComponent(tag)}&spk=${encodeURIComponent(spk_G)}&upk=${encodeURIComponent(upk)}`;

    // HMAC Computation
    const mac = createHmac(wasm, sharedSecret, msg);
    // ------------ HMAC Computation [END] -------------

    const response = await axios.post(
      `${tomcatUrl}/joinGroup?tag=${encodeURIComponent(tag)}&spk=${encodeURIComponent(spk_G)}&upk=${encodeURIComponent(upk)}`,
      null,
      {
        headers: {
          "X-Message": msg,
          "X-MAC": mac,
        },
        withCredentials: true,
      },
    );
    //console.log("Logging response");
    //console.log(response);
    res.json(response.data);
  } catch (error) {
    console.error("Error send the upks to Tomcat:", error);
    res.status(500).json({ message: "Failed to send the upks to Tomcat" });
  }
});

// [NEW] POST request to create an election
app.post("/create-election", async (req, res) => {
  let { browser, election_info, upk, u_i } = req.body;
  let sharedSecret = null;

  try {
    // ------------ HMAC Computation [START] -------------
    if (!browser)
      sharedSecret = await deriveSharedSecret(wasm, u_i, thirdPartyPK);
    else {
      sharedSecret = await deriveSharedSecret(
        wasm,
        req.session.userKeys.u_i,
        thirdPartyPK,
      );

      upk = req.session.userKeys.upk;
    }

    console.log("Logging shared secret: " + sharedSecret);

    // HMAC content
    const msg = JSON.stringify(election_info);

    // HMAC Computation
    const mac = createHmac(wasm, sharedSecret, msg);
    // ------------ HMAC Computation [END] -------------

    console.log("Sending post in axios");
    const response = await axios.post(
      `${thirdPartyURL}/create-election?upk=${encodeURIComponent(upk)}`,
      election_info,
      {
        headers: {
          "X-Message": msg,
          "X-MAC": mac,
        },
        withCredentials: true,
      },
    );
    /*console.log("Logging response");
    console.log(response);*/
    res.json(response.data);
  } catch (error) {
    console.error(
      "Error sending the election information to the 3rd party server:",
      error,
    );
    res.status(500).json({
      message: "Error sending the election information to the 3rd party server",
    });
  }
});

// GET request to retrieve the notifications from the server using the /notifications endpoint
app.post("/notifications", async (req, res) => {
  const { upk } = req.body;

  try {
    const response = await axios.get(
      `${tomcatUrl}/notifications?upk=${encodeURIComponent(upk)}`,
      null,
      {
        withCredentials: true,
      },
    );
    res.json(response.data);
  } catch (error) {
    console.error("Error fetching the notifications from Tomcat:", error);
    res
      .status(500)
      .json({ message: "Error fetching the notifications from Tomcat" });
  }
});

// GET request to retrieve the groups from the server using the /groups endpoint
app.post("/groups", async (req, res) => {
  const { upk } = req.body;

  try {
    const response = await axios.get(
      `${tomcatUrl}/groups?upk=${encodeURIComponent(upk)}`,
      null,
      {
        withCredentials: true,
      },
    );
    res.json(response.data);
  } catch (error) {
    console.error("Error fetching the groups from Tomcat:", error);
    res.status(500).json({ message: "Error fetching the groups from Tomcat" });
  }
});

// GET request to retrieve the notifications from the server using the /notifications endpoint
app.post("/elections", async (req, res) => {
  const { upk, epk } = req.body;

  try {
    if (upk != null) {
      const response = await axios.get(
        `${tomcatUrl}/elections?upk=${encodeURIComponent(upk)}`,
        null,
        {
          withCredentials: true,
        },
      );
      res.json(response.data);
    } else {
      const response = await axios.get(
        `${tomcatUrl}/elections?spk=${encodeURIComponent(spk)}`,
        null,
        {
          withCredentials: true,
        },
      );
      res.json(response.data);
    }
  } catch (error) {
    console.error("Error fetching the elections from Tomcat:", error);
    res
      .status(500)
      .json({ message: "Error fetching the elections from Tomcat" });
  }
});

// GET request to retrieve the election information from the server
app.post("/election", async (req, res) => {
  const { epk } = req.body;

  try {
    const response = await axios.get(
      `${tomcatUrl}/election?epk=${encodeURIComponent(epk)}`,
      null,
      {
        withCredentials: true,
      },
    );
    res.json(response.data);
  } catch (error) {
    console.error("Error fetching the election from Tomcat:", error);
    res
      .status(500)
      .json({ message: "Error fetching the election from Tomcat" });
  }
});

// GET request to retrieve the group information from the server
app.post("/group", async (req, res) => {
  const { spk } = req.body;

  try {
    const response = await axios.get(
      `${tomcatUrl}/group?spk=${encodeURIComponent(spk)}`,
      null,
      {
        withCredentials: true,
      },
    );
    res.json(response.data);
  } catch (error) {
    console.error("Error fetching the group from Tomcat:", error);
    res.status(500).json({ message: "Error fetching the group from Tomcat" });
  }
});

// POST request to send hmac to Tomcat
app.post("/hmac", async (req, res) => {
  const { msg, mac, upk } = req.body;

  try {
    const response = await axios.post(
      `${tomcatUrl}/hmac?upk=${encodeURIComponent(upk)}`,
      null,
      {
        headers: {
          "X-Message": msg,
          "X-MAC": mac,
        },
        withCredentials: true,
      },
    );

    // Extract Set-Cookie from Tomcat response and forward it
    const setCookieHeader = response.headers["set-cookie"];
    if (setCookieHeader) {
      res.setHeader("Set-Cookie", setCookieHeader);
    }

    res.json(response.data);
  } catch (error) {
    console.error("Error forwarding hmac to Tomcat:", error);
    res.status(500).json({ message: "Failed to send hmac to Tomcat" });
  }
});

// GET request to the /hello endpoint
app.post("/key", async (req, res) => {
  try {
    const cookieHeader = req.headers.cookie;

    const response = await axios.get(`${tomcatUrl}/key`, {
      headers: {
        Cookie: cookieHeader,
      },
      withCredentials: true,
    });
    res.json(response.data); // Send the response data back to the frontend
  } catch (error) {
    console.error("Error obtaining the key:", error);
    res
      .status(500)
      .json({ message: "Error obtaining the key from the Tomcat server" });
  }
});

// Start the client app
app.listen(port, () => {
  console.log(`Client app running at http://localhost:${port}`);
});

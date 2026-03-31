import axios from "axios";
import { format, parseISO, isBefore } from "date-fns";

// Configure Axios defaults (middleware base URL)
axios.defaults.baseURL = "http://localhost:3000";
axios.defaults.withCredentials = true;

const wasmFolder = "../../binaries/"

// ------------------------ WASM Loader ------------------------
export async function loadWasmFunctions() {
  const createUserMod = await import(/* @vite-ignore */wasmFolder + "create_user.js");
  const tagMod = await import(/* @vite-ignore */wasmFolder + "gen_group_tag.js");
  const voteMod = await import(/* @vite-ignore */wasmFolder + "encrypt_vote.js");
  const groupInitMod = await import(/* @vite-ignore */wasmFolder + "group_init_exp.js");
  const shuffleMod = await import(/* @vite-ignore */wasmFolder + "shuffle_user.js");
  const intersectionMod = await import(/* @vite-ignore */wasmFolder + "intersection.js");
  const cryptoMod = await import(/* @vite-ignore */wasmFolder + "crypto.js");

  // Each WASM file exports an async factory (`.default`)
  const [user, tag, vote, init, shuffle, intersection, crypto] = await Promise.all([
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
    generateTag: tag.cwrap("generate_group_tag", "string", ["string", "string", "string"]),
    encryptVote: vote.cwrap("encrypt_vote", "string", ["string", "string", "int"]),
    groupInitExp: init.cwrap("group_init_exp", "string", ["int", "string"]),
    shuffleUser: shuffle.cwrap("shuffle_user", "string", ["string", "string", "string"]),
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
    }
  };
}

// ------------------------ API Calls ------------------------

// YES
export async function createClient(wasm) {
  const keys = JSON.parse(wasm.generateKeys());
  await axios.post("/send-user-key", { upk: keys.upk_u_i });
  return { ...keys, tag: null };
}

// YES - NEW
export async function createGroup(name, upks) {
  //const upks = members.map((c) => c.upk_u_i);
  console.log("Logging upks in wasmAPI: ");
  console.log(upks);
  const res = await axios.post("/request-group-creation", { browser: true, name: name, members: upks });
  return { spk_G: res.data.spk_G, s_G: res.data.s_G };
}

// YES
export async function generateGroupTag(wasm, spk, upk, v_i) {
  const tag = JSON.parse(
    wasm.generateTag(upk, v_i, spk)
  ).z_i_G;

  return tag;
}

// YES - NEW
export async function confirmGroup(spk, tag, upk, id) {
  return axios.post("/confirm-group", { browser: true, spk, tag, upk, id });
}

// YES - ?
export async function encryptVote(votee_upk, value) {
  const res = await axios.post("/encrypt-vote", { browser: true, votee_upk, value });
  console.log( res.data);
  return res.data.ballot;
}

// YES
export async function sendBallot(upk, ballot, isUser) {
  if (isUser)
    await axios.post("/send-vote", { browser: true, votee: upk, ballot });
  else
    await axios.post("/send-vote", { browser: true, epk_E: upk, ballot });
}

export async function voteForUser(voter, votee_upk, wasm, groupUsers, votes) {
  const score = Math.floor(Math.random() * 10) + 1;
  if (groupUsers.includes(voter)) votes.push(score);

  const ballot = JSON.parse(
    wasm.encryptVote(voter.v_i, votee_upk, score)
  ).ballot;

  await axios.post("/send-vote", { votee: votee_upk, ballot });
}

// YES
export async function createElection(election_info) {
  await axios.post("/create-election", { browser: true, election_info });
}

export async function voteOnElection(wasm, election, voter, votes) {
  const numOptions = election.options.length;
  const choice = Math.floor(Math.random() * numOptions);
  votes.push(choice);

  const ballot = JSON.parse(
    wasm.encryptVote(voter.v_i, election.epk_E, choice)
  ).ballot;

  await axios.post("/send-vote", { epk_E: election.epk_E, ballot });
}

// YES
export async function requestJoinGroup(spk_G) {
  await axios.post("/join-group", {browser: true, spk_G});
}

// YES
export async function getNotifications(upk) {
  const res = await axios.post("/notifications", { upk });
  return res.data;
}

// YES
export async function getGroups(upk) {
  const res = await axios.post("/groups", { upk });
  return res.data;
}

// YES
export async function getGroupElections(spk) {
  const res = await axios.post("/elections", { upk: null, spk });
  return res.data;
}

// YES
export async function getUserElections(upk) {
  const res = await axios.post("/elections", { upk, spk: null });
  return res.data;
}

// YES
export async function getElection(epk) {
  const res = await axios.post("/election", { epk });
  console.log("Sent request with epk " + epk + " to middleware")
  return res.data;
}

// YES
export async function getGroup(spk) {
  const res = await axios.post("/group", { spk });
  console.log("Sent request with spk " + spk + " to middleware")
  return res.data;
}

// ------------------------ API Calls NEW ------------------------

export async function createClient2() {
  const res = await axios.post("/send-user-key", {
    browser: true,
    upk: null
  });

  console.log(res);

  // only public key comes back
  return {
    upk: res.data.upk,
    u_i: res.data.u_i,
    v_i: res.data.v_i,
  };
}

export async function saveClientKeys(upk, u_i, v_i) {
  const res = await axios.post("/save-user-key", {
    upk: upk,
    u_i: u_i,
    v_i: v_i,
  });

  // only public key comes back
  return {
    upk: res.data.upk,
  };
}

// ------------------------ Helpers ------------------------

export function getScore(scores) {
  const sum = scores.reduce((acc, val) => acc + val, 0);
  return sum / scores.length;
}

export function getCount(votes) {
  const counts = {};
  for (const v of votes) {
    counts[v] = (counts[v] || 0) + 1;
  }
  return Object.keys(counts)
    .map((k) => `${k}: ${counts[k]} vote${counts[k] > 1 ? "s" : ""}`)
    .join("; ");
}

export function isElectionExpired(deadlineStr) {
  try {
    const date = parseISO(deadlineStr); // e.g., "2025-09-19T14:57"
    const formatted = format(date, "EEEE, do MMMM, yyyy '@' HH:mm");

    // Compare deadline with now
    const expired = isBefore(date, new Date());

    return expired;
  } catch (err) {
    console.error("Invalid deadline:", deadlineStr, err);
    return deadlineStr;
  }
}

export function formatDeadline(deadlineStr) {
  if (!deadlineStr) return "No deadline set";

  try {
    const date = parseISO(deadlineStr); // e.g., "2025-09-19T14:57"
    const formatted = format(date, "EEEE, do MMMM, yyyy '@' HH:mm");

    // Compare deadline with now
    const expired = isBefore(date, new Date());

    return expired ? `${formatted} (Expired)` : `${formatted} (Upcoming)`;
  } catch (err) {
    console.error("Invalid deadline:", deadlineStr, err);
    return deadlineStr;
  }
}


// ------------------------ Signatures ------------------------
// wasm is the loaded WebAssembly module
// wasm.exports must contain the functions below

export async function deriveSharedSecret(wasm, privateKeyHex, peerPublicKeyHex) {
  return wasm.crypto.deriveSharedSecretHex(privateKeyHex, peerPublicKeyHex);
}

export function createHmac(wasm, sharedSecretHex, message) {
  return wasm.crypto.createHmacHex(sharedSecretHex, message);
}

export function verifyHmac(wasm, sharedSecretHex, message, macHex) {
  return wasm.crypto.verifyHmacHex(sharedSecretHex, message, macHex);
}

// YES
export async function testHMAC(wasm, serverUpk, u_i, upk, msg) {

  const shared = wasm.crypto.deriveSharedSecretHex(
    u_i,
    serverUpk
  );

  console.log("Shared secret " + shared);

  const mac = wasm.crypto.createHmacHex(shared, msg);

  //const ok = wasm.crypto.verifyHmacHex(shared, message, mac);

  await axios.post("/hmac", {
    msg,
    mac: mac,
    upk: upk
  });

  return true;
}

// ---------------------------------

// YES
export async function getServerKey() {

  const res = await post("/key");

  const { pk } = await res.json();

  return pk;
}




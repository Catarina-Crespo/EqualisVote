// benchElection.js
import { spawn } from "child_process";
import fs from "fs";
import path from "path";

// ---------------- CONFIG ----------------
// const groupSizes = [5, 10, 20]; 
//const groupSizes = [5, 10, 20, 40, 60];
//const groupSizes = [100]; 
const groupSizes = [10, 20];
//const groupSizes = [50, 60, 70, 80, 90]; 
//const groupSizes = [100, 120, 140]; 
const repetitions = 10;

// Path to simulation folder
const simFolder = path.resolve("../nyx"); 
const simScript = "simulateME.js";  

// ---------------- RUN SINGLE SIM ----------------
function runSimulation(groupSize) {
  return new Promise((resolve, reject) => {
    const child = spawn("node", [simScript, groupSize], {
      cwd: simFolder,
      shell: true,
    });

    let stdoutData = "";
    let stderrData = "";

    child.stdout.on("data", (data) => {
      const text = data.toString();
      process.stdout.write(text); 
      stdoutData += text;
    });

    child.stderr.on("data", (data) => {
      const text = data.toString();
      process.stderr.write(text);
      stderrData += text;
    });

    child.on("close", (code) => {
      if (code === 0) {
        const metrics = parseMetrics(stdoutData);
        resolve(metrics);
      } else {
        reject(new Error(`Simulation failed with code ${code}\n${stderrData}`));
      }
    });
  });
}

// ---------------- PARSE METRICS ----------------
function parseMetrics(stdout) {
  const lines = stdout.split("\n");

  const metrics = {};
  const bwEntries = [];

  for (let line of lines) {
    line = line.trim();
    if (line.startsWith("Runtime (ms):")) {
      metrics.runtime_ms = parseFloat(line.split(":")[1]);
    } else if (line.startsWith("RAM usage (MB):")) {
      metrics.ram_mb = parseFloat(line.split(":")[1]);
    } else if (line.startsWith("Vote encryption avg")) {
      metrics.vote_encryption_avg_ms = parseFloat(line.split(":")[1]);
    } else if (line.startsWith("Intersection latency")) {
      metrics.intersection_ms = parseFloat(line.split(":")[1]);
    } else if (line.startsWith("Group creation latency")) {
      metrics.group_creation_ms = parseFloat(line.split(":")[1]);
    } else if (line.startsWith("User") && line.includes("Sent:")) {
      const match = line.match(
        /User (\w+)\.\.\. \| Sent: (\d+)B \| Received: (\d+)B/
      );
      if (match) {
        const upk = match[1];
        const sent = parseInt(match[2]);
        const received = parseInt(match[3]);
        bwEntries.push({ upk, sent, received });
      }
    }
  }

  // Sort by received descending
  bwEntries.sort((a, b) => b.received - a.received);

  // Store top 2
  const top2 = bwEntries.slice(0, 2);
  if (top2[0]) {
    metrics.top1_upk = top2[0].upk;
    metrics.top1_sent = top2[0].sent;
    metrics.top1_received = top2[0].received;
  }
  if (top2[1]) {
    metrics.top2_upk = top2[1].upk;
    metrics.top2_sent = top2[1].sent;
    metrics.top2_received = top2[1].received;
  }

  return metrics;
}

// ---------------- RUN BENCHMARK ----------------
async function runBenchmark() {
  const results = [];

  for (const groupSize of groupSizes) {
    for (let rep = 1; rep <= repetitions; rep++) {
      console.log(`\n[RUN] Group size=${groupSize}, repetition=${rep}`);
      try {
        const metrics = await runSimulation(groupSize);
        results.push({ groupSize, repetition: rep, ...metrics });
      } catch (err) {
        console.error(err.message);
      }
    }
  }

  // CSV header
  const csvHeader = [
    "groupSize",
    "repetition",
    "runtime_ms",
    "ram_mb",
    "vote_encryption_avg_ms",
    "intersection_ms",
    "group_creation_ms",
    "top1_upk",
    "top1_sent",
    "top1_received",
    "top2_upk",
    "top2_sent",
    "top2_received",
  ];

  const csvLines = results.map((r) => {
    return `${r.groupSize},${r.repetition},${r.runtime_ms || ""},${
      r.ram_mb || ""
    },${r.vote_encryption_avg_ms || ""},${r.intersection_ms || ""},${
      r.group_creation_ms || ""
    },${r.top1_upk || ""},${r.top1_sent || ""},${r.top1_received || ""},${
      r.top2_upk || ""
    },${r.top2_sent || ""},${r.top2_received || ""}`;
  });

  const csvContent = [csvHeader.join(","), ...csvLines].join("\n");
  fs.writeFileSync("election_benchmark.csv", csvContent);
  console.log("\nBenchmark complete. CSV saved as election_benchmark.csv");
}

runBenchmark();

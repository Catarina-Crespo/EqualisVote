import { spawn } from "child_process";
import fs from "fs";
import path from "path";

// ---------------- CONFIG ----------------
//const groupSizes = [5, 10 ,20, 30, 40];
//const groupSizes = [50, 60, 70, 80, 90]; // group members sizes
const groupSizes = [10, 20]; // group members sizes
//const groupSizes = [140, 160]; // group members sizes

//const groupSizes = [5, 10, 20];
//const groupSizes = [20, 40, 60, 80];
//const groupSizes = [100, 160];
const usersN = 100;

const repetitions = 10; // runs per group size
const totalUsersMultiplier = 1.5; // e.g., n_voting_users = groupSize * 2

// Path to your simulation folder
const simFolder = path.resolve("../nyx");
const simScript = "simulateMU.js"; 

// ---------------- RUN SINGLE SIM ----------------
function runSimulation(users, groupSize) {
  return new Promise((resolve, reject) => {
    const child = spawn("node", [simScript, groupSize, users], {
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
        // Parse metrics from stdout
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
      // User abcdef12... | Sent: 1234B | Received: 4321B
      const match = line.match(
        /User (\w+)\.\.\. \| Sent: (\d+)B \| Received: (\d+)B/
      );
      if (match) {
        const upk = match[1];
        const sent = parseInt(match[2]);
        const received = parseInt(match[3]);
        metrics[`bw_${upk}`] = { sent, received };
      }
    }
  }
  return metrics;
}

// ---------------- RUN BENCHMARK ----------------
async function runBenchmark() {
  const results = [];

  for (const groupSize of groupSizes) {
    for (let rep = 1; rep <= repetitions; rep++) {
      console.log(`\n[RUN] Group size=${groupSize}, repetition=${rep}`);
      //const users = groupSize * totalUsersMultiplier;
      const users = usersN;

      try {
        const metrics = await runSimulation(users, groupSize);
        results.push({ users, groupSize, repetition: rep, ...metrics });
      } catch (err) {
        console.error(err.message);
      }
    }
  }

  // CSV header
  const csvHeader = [
    "users",
    "groupSize",
    "repetition",
    "runtime_ms",
    "ram_mb",
    "vote_encryption_avg_ms",
    "intersection_ms",
    "group_creation_ms",
    "bw_user1_sent",
    "bw_user1_received",
    "external_user_sent",
    "external_user_received",
    "bw_user3_sent",
    "bw_user3_received",
  ];

  const csvLines = results.map((r) => {
    // Get all bandwidth entries
    const bwEntries = Object.keys(r)
      .filter((k) => k.startsWith("bw_"))
      .map((k) => ({ upk: k, ...r[k] }));

    // Sort by received bytes descending
    bwEntries.sort((a, b) => b.received - a.received);

    // Pick top 3
    const top3 = bwEntries.slice(0, 3);

    // Build values and assign labels 
    const bwValues = top3
      .map((entry, idx) => {
        let label;
        if (idx === 1) {
          // second highest received is external user
          label = "external_user";
        } else {
          label = `bw_user${idx === 0 ? 1 : 3}`; // top1 -> bw_user1, top3 -> bw_user3
        }
        return `${entry.sent},${entry.received}`;
      })
      .join(",");

    return `${r.users},${r.groupSize},${r.repetition},${r.runtime_ms || ""},${
      r.ram_mb || ""
    },${r.vote_encryption_avg_ms || ""},${r.intersection_ms || ""},${
      r.group_creation_ms || ""
    },${bwValues}`;
  });

  const csvContent = [csvHeader.join(","), ...csvLines].join("\n");
  fs.writeFileSync("user_benchmark.csv", csvContent);
  console.log("\nBenchmark complete. CSV saved as user_benchmark.csv");
}

runBenchmark();

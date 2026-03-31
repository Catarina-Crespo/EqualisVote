import pandas as pd
import matplotlib.pyplot as plt

# File to compare the election dataset with the corresponding simulation in user voting

# Load both datasets
df_election = pd.read_csv("election_data.csv")
df_user_eq = pd.read_csv("user_equals_data.csv")

# Group and average repetitions
grouped_election = df_election.groupby("groupSize").mean(numeric_only=True).reset_index()
grouped_user_eq = df_user_eq.groupby("groupSize").mean(numeric_only=True).reset_index()

# -------- Plot 1: Runtime --------
plt.figure(figsize=(10, 6))

plt.plot(grouped_election["groupSize"], grouped_election["runtime_ms"],
         marker="o", linestyle='-', color='tab:blue', label="Election Protocol")

plt.plot(grouped_user_eq["groupSize"], grouped_user_eq["runtime_ms"],
         marker="s", linestyle='-', color='tab:orange', label="User Protocol")

plt.xlabel("Group Size/Number of Votes")
plt.ylabel("Average Runtime (ms)")
plt.title("Runtime Comparison")
plt.grid(True)
plt.legend()
plt.tight_layout()
plt.show()


# -------- Plot 2: RAM --------
plt.figure(figsize=(10, 6))

plt.plot(grouped_election["groupSize"], grouped_election["ram_mb"],
         marker="o", linestyle='-', color='tab:blue', label="Election Protocol")

plt.xlabel("Group Size/Number of Votes")
plt.ylabel("RAM Usage (MB)")
plt.title("RAM Usage Comparison")
plt.grid(True)
plt.legend()
plt.tight_layout()
plt.show()


# -------- Plot 3: Latencies --------
plt.figure(figsize=(10, 6))

# Election
plt.plot(grouped_election["groupSize"], grouped_election["intersection_ms"],
         marker="^", linestyle='-', color='tab:blue', label="Decryption (Election)")

plt.plot(grouped_election["groupSize"], grouped_election["group_creation_ms"],
         marker="x", linestyle='--', color='tab:blue', label="Group Creation (Election)")

plt.xlabel("Group Size/Number of Votes")
plt.ylabel("Latency (ms)")
plt.title("Latency Comparison")
plt.grid(True)
plt.legend()
plt.tight_layout()
plt.show()

# -------- Bandwidth Table --------
bw_cols = [
    "top1_sent", "top1_received",
    "top2_sent", "top2_received"
]

bw_grouped = grouped_election[["groupSize"] + bw_cols]

print("\n=== Average Bandwidth Usage of Top 2 Users (bytes) ===")
print(bw_grouped.to_string(index=False))
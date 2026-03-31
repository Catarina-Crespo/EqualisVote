import pandas as pd
import matplotlib.pyplot as plt

# Old plot file

# Load CSV
df = pd.read_csv("simulation_benchmark.csv")


# Group by users and groupSize (to handle repetitions) and take mean
grouped = df.groupby(["users", "groupSize"]).mean(numeric_only=True).reset_index()

# -------- Overlapped Runtime Plot --------
plt.figure(figsize=(8, 5))

# Runtime vs Users
plt.plot(grouped["users"], grouped["runtime_ms"], marker="o", label="Runtime vs Users")

# Runtime vs Group Size
plt.plot(grouped["groupSize"], grouped["runtime_ms"], marker="s", label="Runtime vs Group Size")

plt.xlabel("Users / Group Size")
plt.ylabel("Average Runtime (ms)")
plt.title("Average Runtime vs Users & Group Size")
plt.grid(True)
plt.legend()
plt.tight_layout()
plt.show()


# -------- Plot 2: RAM --------
plt.figure(figsize=(8, 5))

# RAM vs Users
plt.plot(grouped["users"], grouped["ram_mb"], marker="o", label="RAM vs Users")

# RAM vs Group Size
plt.plot(grouped["groupSize"], grouped["ram_mb"], marker="s", label="RAM vs Group Size")

plt.xlabel("Users / Group Size")
plt.ylabel("RAM (MB)")
plt.title("RAM vs Users & Group Size")
plt.grid(True)
plt.legend()
plt.tight_layout()
plt.show()


# -------- Plot 3: Intersection vs Group Creation Latency --------
plt.figure(figsize=(8, 5))
plt.plot(grouped["users"], grouped["intersection_ms"], marker="^", label="Intersection Latency (ms)")
plt.plot(grouped["users"], grouped["group_creation_ms"], marker="v", label="Group Creation Latency (ms)")
plt.xlabel("Users")
plt.ylabel("Latency (ms)")
plt.title("Intersection & Group Creation Latency vs Users")
plt.grid(True)
plt.legend()
plt.tight_layout()
plt.show()

# -------- Bandwidth Table --------
bw_cols = [
    "bw_user1_sent", "bw_user1_received",
    "external_user_sent", "external_user_received",
    "bw_user3_sent", "bw_user3_received"
]

bw_grouped = grouped[["users", "groupSize"] + bw_cols]

print("\n=== Average Bandwidth Usage per User ===")
print(bw_grouped.to_string(index=False))

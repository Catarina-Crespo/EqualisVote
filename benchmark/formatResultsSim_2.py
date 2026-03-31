import pandas as pd
import matplotlib.pyplot as plt

# File to plot graphs for the different number of votes in the user-voting setting

# Load CSV data
df = pd.read_csv("user_data.csv")  

# Group by 'users' and 'groupSize', and calculate mean for each combination (to handle repetitions)
grouped = df.groupby(["users", "groupSize"]).mean(numeric_only=True).reset_index()

# -------- Overlapped Runtime Plot --------
plt.figure(figsize=(10, 6))

# Filter data for users = 40 and users = 160
data_40 = grouped[grouped["users"] == 40]
data_100 = grouped[grouped["users"] == 100]
data_160 = grouped[grouped["users"] == 160]

# Plot runtime for users = 40 and users = 160
plt.plot(data_40["groupSize"], data_40["runtime_ms"], marker="o", label="Votes = 40", linestyle='-', color='c')
plt.plot(data_100["groupSize"], data_100["runtime_ms"], marker="x", label="Votes = 100", linestyle='-', color='tab:olive')
plt.plot(data_160["groupSize"], data_160["runtime_ms"], marker="s", label="Votes = 160", linestyle='-', color='tab:orange')

plt.xlabel("Group Size")
plt.ylabel("Average Runtime (ms)")
plt.title("Average Runtime vs Group Size (Votes = 40, 100 and 160)")
plt.grid(True)
plt.legend()
plt.tight_layout()
plt.show()

# -------- Plot 2: RAM --------
plt.figure(figsize=(10, 6))

# Plot RAM for users = 40 and users = 160
plt.plot(data_40["groupSize"], data_40["ram_mb"], marker="o", label="Votes = 40", linestyle='-', color='c')
plt.plot(data_100["groupSize"], data_100["ram_mb"], marker="x", label="Votes = 100", linestyle='-', color='tab:olive')
plt.plot(data_160["groupSize"], data_160["ram_mb"], marker="s", label="Votes = 160", linestyle='-', color='tab:orange')

plt.xlabel("Group Size")
plt.ylabel("RAM (MB)")
plt.title("RAM Usage vs Group Size (Votes = 40, 100 and 160)")
plt.grid(True)
plt.legend()
plt.tight_layout()
plt.show()

# -------- Plot 3: Intersection & Group Creation Latency --------
plt.figure(figsize=(10, 6))

# Plot Intersection and Group Creation Latency for users = 40 and users = 160
plt.plot(data_40["groupSize"], data_40["intersection_ms"], marker="^", label="Decryption Latency (Votes = 40)", linestyle='-', color='c')
plt.plot(data_100["groupSize"], data_100["intersection_ms"], marker="s", label="Decryption Latency (Votes = 100)", linestyle='-', color='tab:olive')
plt.plot(data_160["groupSize"], data_160["intersection_ms"], marker="v", label="Decryption Latency (Votes = 160)", linestyle='-', color='tab:orange')
plt.plot(data_40["groupSize"], data_40["group_creation_ms"], marker="x", label="Group Creation Latency (Votes = 40)", linestyle='--', color='c')
plt.plot(data_100["groupSize"], data_100["group_creation_ms"], marker="s", label="Group Creation Latency (Votes = 100)", linestyle='--', color='tab:olive')
plt.plot(data_160["groupSize"], data_160["group_creation_ms"], marker="d", label="Group Creation Latency (Votes = 160)", linestyle='--', color='tab:orange')

plt.xlabel("Group Size")
plt.ylabel("Latency (ms)")
plt.title("Decryption & Group Creation Latency vs Group Size (Votes = 40, 100 and 160)")
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
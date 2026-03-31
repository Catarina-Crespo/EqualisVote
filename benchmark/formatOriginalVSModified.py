import pandas as pd
import matplotlib.pyplot as plt

# File to compare the original implementation with our modified version

# Load both datasets
df_original = pd.read_csv("original_data.csv")
df_user = pd.read_csv("user_data.csv")

# -----------------------------
# Filter only users = 100
# -----------------------------
df_original_100 = df_original[df_original["users"] == 100]
df_user_100 = df_user[df_user["users"] == 100]

# -----------------------------
# Group (handle repetitions)
# -----------------------------
original_grouped = (
    df_original_100
    .groupby(["groupSize"])
    .mean(numeric_only=True)
    .reset_index()
)

user_grouped = (
    df_user_100
    .groupby(["groupSize"])
    .mean(numeric_only=True)
    .reset_index()
)

# -----------------------------
# Plot: Runtime Comparison
# -----------------------------
plt.figure(figsize=(10, 6))

plt.plot(
    original_grouped["groupSize"],
    original_grouped["runtime_ms"],
    marker="o",
    linestyle="-",
    color="tab:blue",
    label="Original Implementation"
)

plt.plot(
    user_grouped["groupSize"],
    user_grouped["runtime_ms"],
    marker="s",
    linestyle="-",
    color="tab:orange",
    label="Modified Implementation"
)

plt.xlabel("Group Size")
plt.ylabel("Average Runtime (ms)")
plt.title("Runtime Comparison vs Group Size (Votes = 100)")
plt.grid(True)
plt.legend()
plt.tight_layout()
plt.show()

# -----------------------------
# Intersection Comparison 
# -----------------------------
if "intersection_ms" in df_user.columns:
    plt.figure(figsize=(10, 6))

    plt.plot(
        original_grouped["groupSize"],
        original_grouped["intersection_ms"],
        marker="o",
        linestyle="-",
        color="tab:blue",
        label="Decryption (Original)"
    )

    plt.plot(
        user_grouped["groupSize"],
        user_grouped["intersection_ms"],
        marker="^",
        linestyle="-",
        color="tab:orange",
        label="Decryption (Modified)"
    )

    plt.xlabel("Group Size")
    plt.ylabel("Latency (ms)")
    plt.title("Decryption Latency vs Group Size (Votes = 100)")
    plt.grid(True)
    plt.legend()
    plt.tight_layout()
    plt.show()
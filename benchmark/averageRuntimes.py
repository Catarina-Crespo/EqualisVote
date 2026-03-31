import pandas as pd

# File to check with pandas how the runtime increases between our implementation and the original one
# It also checks the decryption latency, here called intersection

# Load datasets
new_df = pd.read_csv("user_data.csv")
old_df = pd.read_csv("original_data.csv")

# Filter only for users = 100
new_df = new_df[new_df["users"] == 100]
old_df = old_df[old_df["users"] == 100]

# Compute average runtime and intersection time per groupSize
new_avg = new_df.groupby("groupSize")["runtime_ms"].mean()
old_avg = old_df.groupby("groupSize")["runtime_ms"].mean()

new_intersection_avg = new_df.groupby("groupSize")["intersection_ms"].mean()
old_intersection_avg = old_df.groupby("groupSize")["intersection_ms"].mean()

# Compute runtime percentage spent in intersection
new_intersection_pct = (new_intersection_avg / new_avg) * 100
old_intersection_pct = (old_intersection_avg / old_avg) * 100

# Compute percentage increase in runtime
pct_increase_runtime = (new_avg - old_avg) / old_avg * 100

# Compute percentage increase in intersection proportion
pct_increase_intersection = (new_intersection_pct - old_intersection_pct) / old_intersection_pct * 100

# Combine results in a DataFrame
result_df = pd.DataFrame({
    "groupSize": new_avg.index,
    "old_avg_runtime_ms": old_avg.values,
    "new_avg_runtime_ms": new_avg.values,
    "runtime_pct_increase": pct_increase_runtime.values,
    "old_intersection_pct": old_intersection_pct.values,
    "new_intersection_pct": new_intersection_pct.values,
    "intersection_pct_increase": pct_increase_intersection.values
})

# Print results in requested sentence format
for idx, row in result_df.iterrows():
    print(f"For groupSize = {int(row['groupSize'])}, the average runtime increased by {row['runtime_pct_increase']:.2f}%, "
          f"{row['old_intersection_pct']:.2f}% of the runtime in the original protocol was spent during intersection, "
          f"while for the modified version it was {row['new_intersection_pct']:.2f}% "
          f"(this was an increment of {row['intersection_pct_increase']:.2f}%)")
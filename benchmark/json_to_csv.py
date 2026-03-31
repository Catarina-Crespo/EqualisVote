import json
import csv

# File to convert the output of the original protocol to a csv in the format used to compute the plots

# Input / Output files
input_file = "bench2.json"
output_file = "output2.csv"

# CSV header
header = [
    "users", "groupSize", "repetition",
    "runtime_ms", "ram_mb", "vote_encryption_avg_ms",
    "intersection_ms", "group_creation_ms",
    "bw_user1_sent", "bw_user1_received",
    "external_user_sent", "external_user_received",
    "bw_user3_sent", "bw_user3_received"
]

# Load JSON
with open(input_file, "r") as f:
    data = json.load(f)

rows = []

# Process JSON
for key, repetitions in data.items():
    # Parse key: "groupSize_users_10"
    parts = key.split("_")
    group_size = int(parts[0])
    users = int(parts[1])

    for i, entry in enumerate(repetitions, start=1):
        runtime = entry.get("runtime", {})

        row = [
            users,
            group_size,
            i,  # repetition number
            runtime.get("full_protocol", 0),   # runtime_ms
            0,  # ram_mb (not available)
            0,  # vote_encryption_avg_ms
            runtime.get("intersection", 0),    # intersection_ms
            0,  # group_creation_ms
            0,  # bw_user1_sent
            0,  # bw_user1_received
            0,  # external_user_sent
            0,  # external_user_received
            0,  # bw_user3_sent
            0   # bw_user3_received
        ]

        rows.append(row)

# Write CSV
with open(output_file, "w", newline="") as f:
    writer = csv.writer(f)
    writer.writerow(header)
    writer.writerows(rows)

print(f"CSV file written to {output_file}")
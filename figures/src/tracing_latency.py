import matplotlib.pyplot as plt
import numpy as np
import os

def read_latency_stats(filepath):
    """Reads a file and computes average and 99th percentile latency."""
    with open(filepath, 'r') as f:
        values = [float(line.strip()) for line in f if line.strip()]
    if len(values) < 1000:
        raise ValueError(f"{filepath} does not contain 1000 values.")
    avg = np.mean(values)
    p99 = sorted(values)[990]
    return avg, p99

# File paths
file_paths = {
    "GET /hotels - Counters": "../jaeger_latency_extraction/Output/Counters/GET_hotels_latencies.txt",
    "GET /hotels - NoCounters": "../jaeger_latency_extraction/Output/NoCounters/GET_hotels_latencies.txt",
    "GET /recs - Counters": "../jaeger_latency_extraction/Output/Counters/GET_recommendations_latencies.txt",
    "GET /recs - NoCounters": "../jaeger_latency_extraction/Output/NoCounters/GET_recommendations_latencies.txt"
}

# Read stats
hotels_avg, hotels_p99 = read_latency_stats(file_paths["GET /hotels - Counters"])
hotels_avg_nc, hotels_p99_nc = read_latency_stats(file_paths["GET /hotels - NoCounters"])
recs_avg, recs_p99 = read_latency_stats(file_paths["GET /recs - Counters"])
recs_avg_nc, recs_p99_nc = read_latency_stats(file_paths["GET /recs - NoCounters"])

# Create independent subplots
fig, axs = plt.subplots(1, 2, figsize=(12, 6), sharey=False)  # <- Ensures separate Y axes

bar_width = 0.35
x = np.arange(2)  # Avg and P99

# GET /hotels
axs[0].bar(x - bar_width/2, [hotels_avg, hotels_p99], width=bar_width, label="With Counters", color='skyblue')
axs[0].bar(x + bar_width/2, [hotels_avg_nc, hotels_p99_nc], width=bar_width, label="No Counters", color='orange')
axs[0].set_xticks(x)
axs[0].set_xticklabels(["Avg", "P99"])
axs[0].set_title("GET /hotels")
axs[0].set_ylabel("Latency (ms)")
axs[0].grid(axis='y', linestyle='--', alpha=0.5)
axs[0].legend(loc='upper left', bbox_to_anchor=(0.02, 0.98))  # Legend in upper left for GET /hotels

# GET /recs
axs[1].bar(x - bar_width/2, [recs_avg, recs_p99], width=bar_width, label="With Counters", color='skyblue')
axs[1].bar(x + bar_width/2, [recs_avg_nc, recs_p99_nc], width=bar_width, label="No Counters", color='orange')
axs[1].set_xticks(x)
axs[1].set_xticklabels(["Avg", "P99"])
axs[1].set_title("GET /recommendations")
axs[1].grid(axis='y', linestyle='--', alpha=0.5)
axs[1].legend(loc='upper left', bbox_to_anchor=(0.02, 0.98))  # Legend in upper right for GET /recs

# Title and layout
fig.suptitle("Latency Comparison: With vs Without Counters")
plt.tight_layout(rect=[0, 0, 1, 1])  # Adjusted tight layout for better spacing
plt.savefig("../plots/latency_subfigures.png")
plt.show()

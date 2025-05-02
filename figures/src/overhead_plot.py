import matplotlib.pyplot as plt
import numpy as np
import os

def read_metrics(filepath):
    """Reads values from a file and computes average and 99th percentile."""
    with open(filepath, 'r') as f:
        values = [float(line.strip()) for line in f if line.strip()]
    if not values or len(values) < 1000:
        return None
    avg = np.mean(values)
    p99 = sorted(values)[990]
    return avg, p99

def construct_filename(monitoring, mode, iter_num, version=None):
    """Builds filename given parameters."""
    if monitoring == "no_monitoring":
        return f"../data/no_monitoring_output_{mode}_{iter_num}.txt"
    return f"../data/{monitoring}_output_{mode}_{iter_num}_{version}.txt"

def label_to_number(label):
    """Converts '1K' -> '1000', '10M' -> '10000000'."""
    if label[-1] in 'KkMm':
        base = int(label[:-1])
        mult = 1_000 if label[-1].upper() == 'K' else 1_000_000
        return str(base * mult)
    return label

def overhead_plotter(mode, type_res):
    iteration_labels = {
        "mem": ["1K", "10K", "100K", "1M", "10M"],
        "cpu": ["1K", "10K", "100K", "1M", "10M"],
        "io":  ["1", "10", "100", "1K", "10K"]
    }
    versions = ["2", "4"]
    
    overhead = {
        "papi": {v: [] for v in versions},
        "perf": {v: [] for v in versions}
    }

    valid_labels = []

    for label in iteration_labels[mode]:
        iter_num = label_to_number(label)

        no_mon_file = construct_filename("no_monitoring", mode, iter_num)
        if not os.path.exists(no_mon_file):
            print(f"Missing base file: {no_mon_file}")
            continue

        base_metrics = read_metrics(no_mon_file)
        if base_metrics is None:
            continue
        base_val = base_metrics[type_res]

        all_found = True
        for v in versions:
            papi_file = construct_filename("papi", mode, iter_num, v)
            perf_file = construct_filename("perf", mode, iter_num, v)
            if not (os.path.exists(papi_file) and os.path.exists(perf_file)):
                print(f"Missing: {papi_file} or {perf_file}")
                all_found = False
                break
        if not all_found:
            continue

        for v in versions:
            papi_metrics = read_metrics(construct_filename("papi", mode, iter_num, v))
            perf_metrics = read_metrics(construct_filename("perf", mode, iter_num, v))
            if papi_metrics is None or perf_metrics is None:
                continue
            overhead["papi"][v].append(papi_metrics[type_res] - base_val)
            overhead["perf"][v].append(perf_metrics[type_res] - base_val)

        valid_labels.append(label)

    if not valid_labels:
        print(f"No valid data to plot overhead for mode={mode}")
        return

    x = np.arange(len(valid_labels))
    plt.figure(figsize=(8, 5))

    for v, color, style in zip(versions, ['r', 'm'], ['-', '--']):
        plt.plot(x, overhead["papi"][v], marker='o', linestyle=style, color=color, label=f"PAPI-{v} Metrics")
        plt.plot(x, overhead["perf"][v], marker='s', linestyle=style, color='b' if v == '2' else 'c', label=f"perf-{v} Metrics")

    plt.xticks(x, valid_labels)
    plt.yscale("symlog", linthresh=0.1)
    plt.xlabel("Iterations")
    plt.ylabel("Overhead (ms)")
    
    if type_res == 0:
        title_metric = "Average"
    elif type_res == 1:
        title_metric = "99th Percentile"
    else:
        print("Invalid type_res. Use 0 (avg) or 1 (p99).")
        return

    plt.title(f"{title_metric} Monitoring Overhead for {mode.upper()}")
    plt.grid(True, linestyle="--", alpha=0.5)
    plt.legend()
    plt.tight_layout()
    plt.savefig(f"../plots/overhead_{mode}_{title_metric.lower().replace(' ', '_')}.png")
    plt.show()

# Generate plots for average and 99th percentile overhead
for type_res in [0, 1]:
    for mode in ["mem", "cpu", "io"]:
        overhead_plotter(mode, type_res)

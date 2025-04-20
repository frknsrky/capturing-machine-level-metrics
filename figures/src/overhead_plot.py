import matplotlib.pyplot as plt
import numpy as np

def overhead_plotter(mode):
    if(mode=="mem"):
    
        # Data points (Execution Time in ms)
        papi = {
            "1K": (0.076, 0.116),
            "10K": (0.521, 0.605),
            "100K": (5.126, 5.428),
            "1M": (46.496, 52.279),
            "10M": (459.070, 510.373),
        }
       
        perf = {
            "1K": (0.076, 0.105),
            "10K": (0.521, 0.535),
            "100K": (5.126, 4.921),
            "1M": (46.496, 47.832),
            "10M": (459.070, 471.875),
        }
       
        iterations = ["1K", "10K", "100K", "1M", "10M"]
        
    elif(mode=="cpu"):
        # Data points (Execution Time in ms)
        papi = {
            "1K": (0.115, 0.151),
            "10K": (1.107, 1.115),
            "100K": (10.987, 11.140),
            "1M": (110.398, 110.039),
            "10M": (1098.764, 1101.374),
        }
       
        perf = {
            "1K": (0.115, 0.169),
            "10K": (1.107, 1.239),
            "100K": (10.987, 11.256),
            "1M": (110.398, 112.450),
            "10M": (1098.764, 1113.285),
        } 
        
        iterations = ["1K", "10K", "100K", "1M", "10M"]
       
    elif(mode=="io"):
        # Data points (Execution Time in ms)
        papi = {
            "1": (0.222, 0.295),
            "10": (0.256, 0.324),
            "100": (2.056, 2.177),
            "1K": (20.033, 20.316),
            "10K": (275.148, 242.150),
        }
       
        perf = {
            "1": (0.222, 0.227),
            "10": (0.256, 0.442),
            "100": (2.056, 2.506),
            "1K": (20.033, 23.984),
            "10K": (275.148, 254.031),
        }
        
        iterations = ["1","10","100","1K", "10K"]

    # Extracting x values (iterations)
    x_values = np.arange(len(iterations))  # Numeric indices for categorical x-axis
    
    # Extracting y values
    papi_off = np.array([v[0] for v in papi.values()])
    papi_on = np.array([v[1]-v[0] for v in papi.values()])
    perf_off = np.array([v[0] for v in perf.values()])
    perf_on = np.array([v[1]-v[0] for v in perf.values()])
    
    # Apply slight offsets to reduce overlap
    offset = 0.03  # Adjust this value if needed
    papi_off += offset
    perf_off -= offset
    
    # Plot the lines
    plt.figure(figsize=(6, 4))
    plt.plot(x_values, papi_on, marker="o", linestyle="-", color="r", label="PAPI", alpha=0.8)
    plt.plot(x_values, perf_on, marker="s", linestyle="-", color="b", label="perf", alpha=0.8)
    
    # Set x-ticks to categorical values
    plt.xticks(x_values, iterations)
    
    plt.yscale("log")
    
    # Labels and title
    plt.xlabel("Iterations")
    plt.ylabel("Difference of Execution Time (ms)")
    plt.title("Latency Impact of PAPI and perf Monitoring")
    plt.legend()
    plt.grid(True, linestyle="--")
    
    # Show plot
    plt.savefig(f"../plots/overhead_{mode}.png", bbox_inches='tight')
    plt.show()

modes =["mem","cpu","io"]

for i in modes:
    overhead_plotter(i)


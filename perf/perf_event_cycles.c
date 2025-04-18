#define _GNU_SOURCE
#include <linux/perf_event.h>
#include <asm/unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <string.h>
#include <x86intrin.h>  // Required for __rdtsc()

void some_computation(int iterations) {
    volatile int sum = 0;
    for (int i = 0; i < iterations; i++) {
        sum += i;  // Simple computation to generate instructions and cache usage
    }
}

static long perf_event_open(struct perf_event_attr *hw_event, pid_t pid, 
                            int cpu, int group_fd, unsigned long flags) {
    return syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
}

double measure_events(int iterations, int enable_counters) {
    unsigned long long start, end;
    double runtime_cycles = 0;

    if (enable_counters == 0) {
        start = __rdtsc();
        
        // Sample computation
        some_computation(iterations);
        
        end = __rdtsc();
        
        runtime_cycles = (double)(end - start);
    }

    if (enable_counters == 1) {
        struct perf_event_attr pe = {0};

        // Open the leader event (Total CPU Cycles)
        pe.type = PERF_TYPE_HARDWARE;
        pe.size = sizeof(struct perf_event_attr);
        pe.config = PERF_COUNT_HW_CPU_CYCLES;
        pe.disabled = 1;
        pe.exclude_kernel = 1;
        pe.exclude_hv = 1;

        int leader_fd = perf_event_open(&pe, 0, -1, -1, 0);
        if (leader_fd == -1) {
            perror("perf_event_open (leader)");
            return 0;
        }

        // Open follower events in the same group
        pe.disabled = 0; // Followers are enabled when the leader is enabled

        pe.config = PERF_COUNT_HW_INSTRUCTIONS;
        int instructions_fd = perf_event_open(&pe, 0, -1, leader_fd, 0);

        pe.type = PERF_TYPE_HW_CACHE;
        pe.config = PERF_COUNT_HW_CACHE_L1D | 
                    (PERF_COUNT_HW_CACHE_OP_READ << 8) | 
                    (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);
        int l1_misses_fd = perf_event_open(&pe, 0, -1, leader_fd, 0);

        pe.config = PERF_COUNT_HW_CACHE_LL | 
                    (PERF_COUNT_HW_CACHE_OP_READ << 8) | 
                    (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);
        int llc_misses_fd = perf_event_open(&pe, 0, -1, leader_fd, 0);

        pe.config = PERF_COUNT_HW_CACHE_DTLB | 
                    (PERF_COUNT_HW_CACHE_OP_READ << 8) | 
                    (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);
        int tlb_misses_fd = perf_event_open(&pe, 0, -1, leader_fd, 0);

        ioctl(leader_fd, PERF_EVENT_IOC_RESET, 0);
        ioctl(leader_fd, PERF_EVENT_IOC_ENABLE, 0);
    
        start = __rdtsc();

        // Sample computation
        some_computation(iterations);
        
        // Read results
        long long cycles, instructions, l1_misses, llc_misses, tlb_misses;
        read(leader_fd, &cycles, sizeof(long long));
        read(instructions_fd, &instructions, sizeof(long long));
        read(l1_misses_fd, &l1_misses, sizeof(long long));
        read(llc_misses_fd, &llc_misses, sizeof(long long));
        read(tlb_misses_fd, &tlb_misses, sizeof(long long));
        
        end = __rdtsc();

        runtime_cycles = (double)(end - start);
        
        ioctl(leader_fd, PERF_EVENT_IOC_DISABLE, 0);

        // Close file descriptors
        close(leader_fd);
        close(instructions_fd);
        close(l1_misses_fd);
        close(llc_misses_fd);
        close(tlb_misses_fd);
    }

    return runtime_cycles;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <iterations> <enable_counters (0 or 1)>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int iterations = atoi(argv[1]);
    int enable_counters = atoi(argv[2]);

    if (iterations <= 0) {
        fprintf(stderr, "Error: Iterations must be a positive integer.\n");
        return EXIT_FAILURE;
    }
    
    double sum = 0;
    
    for (int i = 0; i < 100; i++) {
        sum += measure_events(iterations, enable_counters);
    }
    
    printf("Avg Execution Cycles: %.3f M cycles, for enable_counters:%d\n", sum / (100.0*1.0e6), enable_counters);
    return EXIT_SUCCESS;
}

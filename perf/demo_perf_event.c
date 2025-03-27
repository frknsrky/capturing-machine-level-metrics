#define _GNU_SOURCE
#include <linux/perf_event.h>
#include <asm/unistd.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <string.h>
#include <time.h>

void some_computation() {
    volatile int sum = 0;
    for (int i = 0; i < 1000000; i++) {
        sum += i;  // Simple computation to generate instructions and cache usage
    }
}

static long perf_event_open(struct perf_event_attr *hw_event, pid_t pid, 
                            int cpu, int group_fd, unsigned long flags) {
    return syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
}

void measure_events() {
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
        return;
    }

    // Open follower events in the same group
    pe.disabled = 0; // Followers are enabled when the leader is enabled

    pe.config = PERF_COUNT_HW_INSTRUCTIONS;
    int instructions_fd = perf_event_open(&pe, 0, -1, leader_fd, 0);

    // L1 Cache
    pe.type = PERF_TYPE_HW_CACHE;
    pe.config = PERF_COUNT_HW_CACHE_L1D | 
                (PERF_COUNT_HW_CACHE_OP_READ << 8) | 
                (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);
    int l1_misses_fd = perf_event_open(&pe, 0, -1, leader_fd, 0);

    // Last Level Cache
    pe.config = PERF_COUNT_HW_CACHE_LL | 
                (PERF_COUNT_HW_CACHE_OP_READ << 8) | 
                (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);
    int llc_misses_fd = perf_event_open(&pe, 0, -1, leader_fd, 0);

    // TLB
    pe.type = PERF_TYPE_HW_CACHE;
    pe.config = PERF_COUNT_HW_CACHE_DTLB | 
                (PERF_COUNT_HW_CACHE_OP_READ << 8) | 
                (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);
    int tlb_misses_fd = perf_event_open(&pe, 0, -1, leader_fd, 0);

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    // Sample computation
    some_computation();

    clock_gettime(CLOCK_MONOTONIC, &end);

    double runtime_ms = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1.0e6;
 
    printf("Execution Time: %.3f ms\n", runtime_ms);
	
    // Start counting
    ioctl(leader_fd, PERF_EVENT_IOC_RESET, 0);
    ioctl(leader_fd, PERF_EVENT_IOC_ENABLE, 0);

    // Sample computation
    some_computation();

    // Stop counting
    ioctl(leader_fd, PERF_EVENT_IOC_DISABLE, 0);

    clock_gettime(CLOCK_MONOTONIC, &end);

    runtime_ms = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1.0e6;
 
    printf("Execution Time: %.3f ms\n", runtime_ms);

    // Read results
    long long cycles, instructions, l1_misses, llc_misses, tlb_misses;
    read(leader_fd, &cycles, sizeof(long long));
    read(instructions_fd, &instructions, sizeof(long long));
    read(l1_misses_fd, &l1_misses, sizeof(long long));
    read(llc_misses_fd, &llc_misses, sizeof(long long));
    read(tlb_misses_fd, &tlb_misses, sizeof(long long));

    // Display results
    printf("Total CPU Cycles: %lld\n", cycles);
    printf("Total Instructions: %lld\n", instructions);
    printf("L1 Data Cache Misses: %lld\n", l1_misses);
    printf("Last Level Cache (LLC) Misses: %lld\n", llc_misses); 
    printf("TLB Misses: %lld\n", tlb_misses);

    // Close file descriptors
    close(leader_fd);
    close(instructions_fd);
    close(l1_misses_fd);
}

int main() {
    measure_events();
    return 0;
}

#define _GNU_SOURCE
#include <linux/perf_event.h>
#include <asm/unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sched.h>
#include <time.h>

#define ARRAY_SIZE (1024 * 1024 * 64) // 64M elements (~256MB for int array)
#define STRIDE 64                     // Stride to introduce cache misses

int *array;

void setup_array() {
    array = (int *)malloc(ARRAY_SIZE * sizeof(int));
    if (!array) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    // Initialize with a stride pattern to cause cache misses
    for (size_t i = 0; i < ARRAY_SIZE; i++) {
        array[i] = (i + STRIDE) % ARRAY_SIZE;
    }
}

void some_computation(long iterations) {
    volatile int sum = 0;
    size_t index = 0;

    for (long i = 0; i < iterations; i++) {
        index = array[index]; // Indirect access causing cache/TLB misses
        sum += index;
    }

    // Prevent compiler optimizations
    printf("Final sum: %d\n", sum);
}

static long perf_event_open(struct perf_event_attr *hw_event, pid_t pid, 
                            int cpu, int group_fd, unsigned long flags) {
    return syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
}

double measure_events(long iterations, int enable_counters) {
	if (enable_counters) {
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
        return -1;
    }

    // Open follower events in the same group
    pe.disabled = 0; // Followers are enabled when the leader is enabled

    pe.config = PERF_COUNT_HW_INSTRUCTIONS;
    int instructions_fd = perf_event_open(&pe, 0, -1, leader_fd, 0);

    // L1 Cache Misses
    pe.type = PERF_TYPE_HW_CACHE;
    pe.config = PERF_COUNT_HW_CACHE_L1D | 
                (PERF_COUNT_HW_CACHE_OP_READ << 8) | 
                (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);
    int l1_misses_fd = perf_event_open(&pe, 0, -1, leader_fd, 0);

    // Last Level Cache (LLC) Misses
    pe.config = PERF_COUNT_HW_CACHE_LL | 
                (PERF_COUNT_HW_CACHE_OP_READ << 8) | 
                (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);
    int llc_misses_fd = perf_event_open(&pe, 0, -1, leader_fd, 0);

    // TLB Misses
    pe.config = PERF_COUNT_HW_CACHE_DTLB | 
                (PERF_COUNT_HW_CACHE_OP_READ << 8) | 
                (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);
    int tlb_misses_fd = perf_event_open(&pe, 0, -1, leader_fd, 0);

  // Pin to a single core to avoid noise from other CPUs
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(0, &set);
    sched_setaffinity(0, sizeof(set), &set);
	}

    setup_array();

    struct timespec start, end;   
    clock_gettime(CLOCK_MONOTONIC, &start);

    if (enable_counters) {

        // Start counting
        ioctl(leader_fd, PERF_EVENT_IOC_RESET, 0);
        ioctl(leader_fd, PERF_EVENT_IOC_ENABLE, 0);

    }

    // Sample computation
    some_computation(iterations);

    if (enable_counters) {

        // Stop counting
        ioctl(leader_fd, PERF_EVENT_IOC_DISABLE, 0);

    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    double runtime_ms = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1.0e6;
    printf("Execution Time: %.3f ms\n", runtime_ms);

    if (enable_counters) {
	
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
        close(llc_misses_fd);
        close(tlb_misses_fd);
    }

  free(array);
  
	return runtime_ms;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <iterations> <enable_counters (0 or 1)>\n", argv[0]);
        return EXIT_FAILURE;
    }

    long iterations = atol(argv[1]);
    int enable_counters = atoi(argv[2]);

    if (iterations <= 0) {
        fprintf(stderr, "Error: Iterations must be a positive integer.\n");
        return EXIT_FAILURE;
    }
	
	double sum=0;
	
	for(int i=0; i<100; i++){
		sum += measure_events(iterations, enable_counters);
	}
	
	printf("Avg Execution Time: %.3f ms, for enable_counters:%d\n", sum/100.0, enable_counters);
    return EXIT_SUCCESS;
}

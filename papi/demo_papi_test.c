#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <unistd.h>
#include <sched.h>
#include <papi.h>

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

double measure_events(long iterations, int enable_counters) {
    int event_set = PAPI_NULL;
    long long values[6];  // Store values for TOT_INS, TOT_CYC, L1_DCM, power, latency, and DRAM access
    struct timespec start, end;  // For runtime measurement
	double runtime_ms;

    // Initialize PAPI library
    if (PAPI_library_init(PAPI_VER_CURRENT) != PAPI_VER_CURRENT) {
        fprintf(stderr, "PAPI library initialization error!\n");
        return 1;
    }

    // Create event set
    if (PAPI_create_eventset(&event_set) != PAPI_OK) {
        fprintf(stderr, "Error creating event set!\n");
        return 1;
    }

    // Add events to monitor
    if (PAPI_add_event(event_set, PAPI_TOT_INS) != PAPI_OK) {
        fprintf(stderr, "Error adding PAPI_TOT_INS event!\n");
        return 1;
    }

    if (PAPI_add_event(event_set, PAPI_TOT_CYC) != PAPI_OK) {
        fprintf(stderr, "Error adding PAPI_TOT_CYC event!\n");
        return 1;
    }

    if (PAPI_add_event(event_set, PAPI_L1_DCM) != PAPI_OK) {
        fprintf(stderr, "Error adding PAPI_L1_DCM event!\n");
        return 1;
    }

    // Memory latency proxy (L2 cache misses)
    if (PAPI_add_event(event_set, PAPI_L2_DCM) != PAPI_OK) {
        fprintf(stderr, "Error adding PAPI_L2_DCM event!\n");
        return 1;
    }

    // DRAM accesses (L3 total cache misses)
    /*if (PAPI_add_event(event_set, PAPI_L3_DCM) != PAPI_OK) {
        fprintf(stderr, "Error adding PAPI_L3_DCM event!\n");
        return 1;
    }*/

  // Pin to a single core to avoid noise from other CPUs
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(0, &set);
    sched_setaffinity(0, sizeof(set), &set);

    setup_array();
  
	if(enable_counters=0){
		clock_gettime(CLOCK_MONOTONIC, &start);

		some_computation(iterations);  // Run the function to measure

		clock_gettime(CLOCK_MONOTONIC, &end);

		runtime_ms = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1.0e6;
 
		printf("Execution Time: %.3f ms\n", runtime_ms);
	}
	
	if(enable_counters=1){
		clock_gettime(CLOCK_MONOTONIC, &start);

		// Start measuring
		PAPI_start(event_set);

		some_computation(iterations);  // Run the function to measure

		// Stop measuring
		PAPI_stop(event_set, values);

		clock_gettime(CLOCK_MONOTONIC, &end);

		runtime_ms = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1.0e6;
	 
		printf("Execution Time: %.3f ms\n", runtime_ms);

		// Print results
		printf("Total instructions: %lld\n", values[0]);
		printf("Total cycles: %lld\n", values[1]);
		printf("L1 Data Cache Misses: %lld\n", values[2]);
		printf("L2 Data Cache Misses: %lld\n", values[3]);
		//printf("L3 Data Cache Misses: %lld\n", values[4
	}

    // Cleanup
    PAPI_shutdown();
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

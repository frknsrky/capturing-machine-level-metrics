#include <stdio.h>
#include <stdlib.h>
#include <papi.h>
#include <time.h>

void some_computation(int iterations) {
    volatile int sum = 0;
    for (int i = 0; i < iterations; i++) {
        sum += i;  // Simple computation to generate instructions and cache usage
    }
}

double measure_events(int iterations, int enable_counters) {
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
	
	return runtime_ms;
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
	
	double sum=0;
	
	for(int i=0; i<100; i++){
		sum += measure_events(iterations, enable_counters);
	}
	
	printf("Avg Execution Time: %.3f ms, for enable_counters:%d\n", sum/100.0, enable_counters);
    return EXIT_SUCCESS;
}

#include <stdio.h>
#include <stdlib.h>
#include <papi.h>
#include <x86intrin.h>  // For __rdtsc()

void some_computation(int iterations) {
    volatile int sum = 0;
    for (int i = 0; i < iterations; i++) {
        sum += i;  // Simple computation to generate instructions and cache usage
    }
}

double measure_events(int iterations, int enable_counters) {
    int event_set = PAPI_NULL;
    long long values[6];  // Store values for TOT_INS, TOT_CYC, L1_DCM, L2_DCM, etc.
    unsigned long long start_tsc, end_tsc;  // TSC timing
    double runtime_cycles;

    if (enable_counters == 0) {
        start_tsc = __rdtsc();

        some_computation(iterations);  // Run the function to measure

        end_tsc = __rdtsc();

        runtime_cycles = (double)(end_tsc - start_tsc);

        printf("Execution Cycles: %.0f cycles\n", runtime_cycles);
    }

    if (enable_counters == 1) {
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

        if (PAPI_add_event(event_set, PAPI_L1_DCM) != PAPI_OK) {
            fprintf(stderr, "Error adding PAPI_L1_DCM event!\n");
            return 1;
        }

        // Memory latency proxy (L2 cache misses)
        if (PAPI_add_event(event_set, PAPI_L2_DCM) != PAPI_OK) {
            fprintf(stderr, "Error adding PAPI_L2_DCM event!\n");
            return 1;
        }
		
		if (PAPI_add_event(event_set, PAPI_TOT_CYC) != PAPI_OK) {
            fprintf(stderr, "Error adding PAPI_TOT_CYC event!\n");
            return 1;
        }

        start_tsc = __rdtsc();
		
		// Start measuring
        PAPI_start(event_set);

        some_computation(iterations);  // Run the function to measure

        // Stop measuring
        PAPI_stop(event_set, values);


        end_tsc = __rdtsc();

        runtime_cycles = (double)(end_tsc - start_tsc);

        printf("Execution Cycles: %.3f cycles\n", runtime_cycles);

        // Print results
        printf("Total cycles: %lld\n", values[3]);
        printf("Total instructions: %lld\n", values[0]);
        printf("L1 Data Cache Misses: %lld\n", values[1]);
        printf("L2 Data Cache Misses: %lld\n", values[2]);

        // Cleanup
        PAPI_shutdown();
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

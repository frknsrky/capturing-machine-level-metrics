#include <stdio.h>
#include <stdlib.h>
#include <papi.h>

void some_computation() {
    volatile int sum = 0;
    for (int i = 0; i < 1000000; i++) {
        sum += i;  // Simple computation to generate instructions and cache usage
    }
}

int main() {
    int event_set = PAPI_NULL;
    long long values[3];  // Store values for TOT_INS, TOT_CYC, and L1_DCM

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

    // Start measuring
    PAPI_start(event_set);

    some_computation();  // Run the function to measure

    // Stop measuring
    PAPI_stop(event_set, values);

    // Print results
    printf("Total instructions: %lld\n", values[0]);
    printf("Total cycles: %lld\n", values[1]);
    printf("L1 Data Cache Misses: %lld\n", values[2]);

    // Cleanup
    PAPI_shutdown();
    return 0;
}

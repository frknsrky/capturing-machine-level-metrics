#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <unistd.h>
#include <sched.h>
#include <papi.h>
#include <math.h>
#include <string.h>

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

void memory_strain(long iterations) {
    volatile int sum = 0;
    size_t index = 0;

    for (long i = 0; i < iterations; i++) {
        index = array[index]; // Indirect access causing cache/TLB misses
        sum += index;
    }

    // Prevent compiler optimizations
    printf("Final sum: %d\n", sum);
}

void cpu_strain(long iterations) {
    volatile double sum = 0.0;
    for (long i = 0; i < iterations; ++i) {
        sum += sin(i) * cos(i) / (tan(i + 1) + 1.0);
    }
	
    // Prevent compiler optimizations
    printf("Final sum: %f\n", sum);
}

void io_strain(long iterations) {
    FILE *file = fopen("io_stress_output.txt", "w");
    if (!file) {
        perror("Error opening file");
        return;
    }

    char *largeBlock = malloc(10001);
    if (!largeBlock) {
        perror("Memory allocation failed");
        fclose(file);
        return;
    }
    memset(largeBlock, 'A', 10000);
    largeBlock[10000] = '\0';

    for (long i = 0; i < iterations; ++i) {
        fprintf(file, "Iteration %ld:\n%s\n", i, largeBlock);
    }

    free(largeBlock);
    fclose(file);
}

double measure_events(long iterations, int enable_counters, char* mode, int num_metrics) {

	void (*compute)(long);
	if (strcmp(mode, "mem") == 0) {
		compute = &memory_strain;
		setup_array();
	} else if (strcmp(mode, "cpu") == 0) {
		compute = &cpu_strain;
	} else {
		compute = &io_strain;
	}

	int event_set = PAPI_NULL;
    	long long values[6];  // Store values for TOT_INS, TOT_CYC, L1_DCM, power, latency, and DRAM access
   	struct timespec start, end;  // For runtime measurement
	double runtime_ms;
	
	if(enable_counters==1){

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

	if (num_metrics == 4) {
	
	    if (PAPI_add_event(event_set, PAPI_L1_DCM) != PAPI_OK) {
	        fprintf(stderr, "Error adding PAPI_L1_DCM event!\n");
	        return 1;
	    }
	
	    // Memory latency proxy (L2 cache misses)
	    if (PAPI_add_event(event_set, PAPI_L3_DCM) != PAPI_OK) {
	        fprintf(stderr, "Error adding PAPI_L2_DCM event!\n");
	        return 1;
	    }
	}

    // DRAM accesses (L3 total cache misses)
    /*if (PAPI_add_event(event_set, PAPI_L3_DCM) != PAPI_OK) {
        fprintf(stderr, "Error adding PAPI_L3_DCM event!\n");
        return 1;
    }*/
	}

  // Pin to a single core to avoid noise from other CPUs
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(0, &set);
    sched_setaffinity(0, sizeof(set), &set);
  
	if(enable_counters==0){
		clock_gettime(CLOCK_MONOTONIC, &start);

		compute(iterations);  // Run the function to measure

		clock_gettime(CLOCK_MONOTONIC, &end);

		runtime_ms = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1.0e6;
 
		//printf("Execution Time: %.3f ms\n", runtime_ms);
	}
	
	if(enable_counters==1){
		clock_gettime(CLOCK_MONOTONIC, &start);

		// Start measuring
		PAPI_start(event_set);

		compute(iterations);  // Run the function to measure

		// Stop measuring
		PAPI_stop(event_set, values);

		clock_gettime(CLOCK_MONOTONIC, &end);

		runtime_ms = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1.0e6;
	 
		//printf("Execution Time: %.3f ms\n", runtime_ms);

		// Print results
		//printf("Total instructions: %lld\n", values[0]);
		//printf("Total cycles: %lld\n", values[1]);
		//printf("L1 Data Cache Misses: %lld\n", values[2]);
		//printf("L2 Data Cache Misses: %lld\n", values[3]);
		//printf("L3 Data Cache Misses: %lld\n", values[4
		// Cleanup
    		PAPI_shutdown();
	}

    if (strcmp(mode, "mem") == 0) {
  	free(array);
    }
	
	return runtime_ms;
}

int main(int argc, char *argv[]) {
    // if (argc != 4) {
    //     fprintf(stderr, "Usage: %s <iterations> <enable_counters (0 or 1)> <mode ('mem', 'cpu', or 'io')>\n", argv[0]);
    //     return EXIT_FAILURE;
    // }

 //    long iterations = atol(argv[1]);
 //    int enable_counters = atoi(argv[2]);
 //    char* mode = argv[3];

 //    if (iterations <= 0) {
 //        fprintf(stderr, "Error: Iterations must be a positive integer.\n");
 //        return EXIT_FAILURE;
 //    }

 //    if (strcmp(mode, "mem") != 0 && strcmp(mode, "cpu") != 0 && strcmp(mode, "io") != 0) {
	// fprintf(stderr, "Error: Mode must be 'mem', 'cpu', or 'io'.\n");
 //        return EXIT_FAILURE; 
 //    }
	
 //    double sum=0;
	
    	char* modes[] = {"mem", "cpu", "io"};

	int j = 0;
	for (; j<2; j++) {
		char* mode = modes[j];
		for (int iterations=1000; iterations<=10000000; iterations *= 10) {
			for (int num_metrics = 4; num_metrics > 0; num_metrics -= 2) {
				char filename[128];
				snprintf(filename, sizeof(filename), "papi_output_%s_%d_%d.txt", mode, iterations, num_metrics);
				FILE *file = fopen(filename, "w");
				for(int i=0; i<1000; i++){
					char str_latency[64];
					sprintf(str_latency, "%f", measure_events(iterations, 1, mode, num_metrics));
					fprintf(file, "%s\n", str_latency);
				}
				fclose(file);
			}
		}
	}
	char* mode = modes[j];
	for (int iterations=1; iterations<=10000; iterations *= 10) {
		for (int num_metrics = 4; num_metrics > 0; num_metrics -= 2) {
			char filename[128];
			snprintf(filename, sizeof(filename), "papi_output_%s_%d_%d.txt", mode, iterations, num_metrics);
			FILE *file = fopen(filename, "w");
			for(int i=0; i<1000; i++){
				char str_latency[64];
				sprintf(str_latency, "%f", measure_events(iterations, 1, mode, num_metrics));
				fprintf(file, "%s\n", str_latency);
			}
			fclose(file);
		}
	}
	
    // printf("Avg Execution Time: %.3f ms, for enable_counters:%d\n", sum/100.0, enable_counters);
    return EXIT_SUCCESS;
}

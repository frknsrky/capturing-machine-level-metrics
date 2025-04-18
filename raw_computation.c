#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <unistd.h>
#include <sched.h>
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

double measure_events(long iterations, int enable_counters, char* mode) {

	void (*compute)(long);
	if (strcmp(mode, "mem") == 0) {
		compute = &memory_strain;
		setup_array();
	} else if (strcmp(mode, "cpu") == 0) {
		compute = &cpu_strain;
	} else {
		compute = &io_strain;
	}

    struct timespec start, end;   
    clock_gettime(CLOCK_MONOTONIC, &start);

    // Sample computation
    compute(iterations);

    clock_gettime(CLOCK_MONOTONIC, &end);

    double runtime_ms = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1.0e6;
    //printf("Execution Time: %.3f ms\n", runtime_ms);

	if (strcmp(mode, "mem") == 0) {
  free(array);
	}
  
	return runtime_ms;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <iterations> <enable_counters (0 or 1)> <mode ('mem', 'cpu', or 'io')>\n", argv[0]);
        return EXIT_FAILURE;
    }

    long iterations = atol(argv[1]);
    int enable_counters = atoi(argv[2]);
    char* mode = argv[3];

    if (iterations <= 0) {
        fprintf(stderr, "Error: Iterations must be a positive integer.\n");
        return EXIT_FAILURE;
    }

    if (strcmp(mode, "mem") != 0 && strcmp(mode, "cpu") != 0 && strcmp(mode, "io") != 0) {
	fprintf(stderr, "Error: Mode must be 'mem', 'cpu', or 'io'.\n");
        return EXIT_FAILURE; 
    }
	
    double sum=0;
	
    for(int i=0; i<100; i++){
	sum += measure_events(iterations, enable_counters, mode);
    }
	
    printf("Avg Execution Time: %.3f ms, for enable_counters:%d\n", sum/100.0, enable_counters);
    return EXIT_SUCCESS;
}

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <x86intrin.h>  // For _mm_clflush
#include <unistd.h>
#include <sys/mman.h>

#define CACHE_SIZE (64 * 1024 * 1024)  // 64MB, larger than most LLCs

void flush_cache() {
    // Allocate a large buffer that exceeds the typical last-level cache size
    size_t buffer_size = CACHE_SIZE;
    uint8_t *buffer = (uint8_t *)malloc(buffer_size);
    
    if (!buffer) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    // Access all memory locations to evict previous cache contents
    for (size_t i = 0; i < buffer_size; i += 64) {
        buffer[i] = i % 256;
    }

    // Use `_mm_clflush()` to explicitly flush cache lines
    for (size_t i = 0; i < buffer_size; i += 64) {
        _mm_clflush(&buffer[i]);  // Flush cache line
    }

    // Ensure all memory writes are committed before continuing
    _mm_mfence();

    // Advise kernel to drop pages (optional, but helps in VMs)
    madvise(buffer, buffer_size, MADV_DONTNEED);

    free(buffer); // Deallocate buffer
}

int main() {
    printf("Flushing cache...\n");
    flush_cache();
    printf("Cache flushed!\n");
    return 0;
}

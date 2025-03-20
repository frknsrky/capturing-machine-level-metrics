#define _GNU_SOURCE
#include <linux/perf_event.h>
#include <asm/unistd.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>

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

long long measure_event(int event) {
    struct perf_event_attr pe = {0};
    pe.type = PERF_TYPE_HARDWARE;
    pe.size = sizeof(struct perf_event_attr);
    pe.config = event;
    pe.disabled = 1;

    int fd = perf_event_open(&pe, 0, -1, -1, 0);
    ioctl(fd, PERF_EVENT_IOC_RESET, 0);
    ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);

    // Sample computation to measure
    some_computation()

    ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);

    long long count;
    read(fd, &count, sizeof(long long));
    close(fd);

    return count;
}

int main() {
    printf("Total Instructions: %lld\n", measure_event(PERF_COUNT_HW_INSTRUCTIONS));
    printf("Total CPU Cycles: %lld\n", measure_event(PERF_COUNT_HW_CPU_CYCLES));

    return 0;
}

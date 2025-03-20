sudo sysctl -w kernel.perf_event_paranoid=0
gcc demo.c -o demo
perf stat -e cycles,instructions,L1-dcache-load-misses,L1-dcache-loads ./demo
To enable perf run: sudo sysctl -w kernel.perf_event_paranoid=0 \n

For linux kernel calls: \n
gcc demo_perf.c -o demo_perf \n
perf stat -e cycles,instructions,L1-dcache-load-misses,L1-dcache-loads ./demo_perf \n 

For using perf_event library in our code (more desirable case):

gcc demo_perf_event.c -o demo_perf_event \n
./demo_perf_event \n
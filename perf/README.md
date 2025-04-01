# PERF
To enable perf run: sudo sysctl -w kernel.perf_event_paranoid=0 

## For linux kernel calls: 

gcc demo_perf.c -o demo_perf 

perf stat -e cycles,instructions,L1-dcache-load-misses,L1-dcache-loads 

./demo_perf 

## For using perf_event library in our code (more desirable case):

gcc demo_perf_event.c -o demo_perf_event 

./demo_perf_event 

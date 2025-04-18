wget http://icl.utk.edu/projects/papi/downloads/papi-6.0.0.tar.gz

tar -xvf papi-6.0.0.tar.gz

cd papi-6.0.0

cd src

./configure --prefix=$PWD/install

make && make install

cd install/bin

./papi_avail

sudo sh -c 'echo -1 >/proc/sys/kernel/perf_event_paranoid'

./papi_avail

cd ../../ctests

./serial_hl

export PAPI_DIR=/users/frknsrky/papi-6.0.0/src/install

export PATH=${PAPI_DIR}/bin:$PATH

export LD_LIBRARY_PATH=${PAPI_DIR}/lib:$LD_LIBRARY_PATH

papi_avail

papi_mem_info

cd /users/frknsrky/

gcc demo_papi.c -I/${PAPI_DIR}/include -L/${PAPI_DIR}/lib -o demo_papi -lpapi

./demo_papi

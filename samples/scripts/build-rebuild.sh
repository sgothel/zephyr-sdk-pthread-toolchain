#!/bin/bash

build_it() {
    # west build -b qemu_cortex_m3 .
    #west build -b mps2/an385 .
    west build -b mps2/an385 . -- -D TEST_ENABLE_LIBSTDCXX_THREADS=1
}

build_it 2>&1 | tee build.log


#!/bin/bash

rm -rf build

build_it() {

    # qemu_cortex_m3 is limited to 256 kB Flash, no MPU
    #west build -p always -b qemu_cortex_m3 .

    # mps2/an385
    # - [Board V2M MPS2 Family](https://docs.zephyrproject.org/latest/boards/arm/mps2/doc/index.html)
    #   - [Board mps2/an385](https://docs.zephyrproject.org/latest/boards/arm/mps2/doc/mps2_armv7m.html#mps2-armv7m-board)
    # - mps2 Armv7-m based board targets supported in Zephyr
    # - ARM Cortex-M3 CPU
    # - MPU suport
    # - 4MB Flash
    #west build -p always -b mps2/an385 .
    west build -p always -b mps2/an385 . -- -D TEST_ENABLE_LIBSTDCXX_THREADS=1

}

build_it 2>&1 | tee build.log

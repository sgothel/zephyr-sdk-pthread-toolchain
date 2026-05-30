#!/bin/bash

sdir=`dirname $(readlink -f $0)`
rootdir=`dirname $sdir`

source ${sdir}/setenv-run.sh

# platform="mps2/an385"
platform="qemu_cortex_r5/zynqmp_rpu"

# west twister -p ${platform} -s posix.pthread.dynamic_stack -T tests/posix_pthread
# west twister -p ${platform} -T tests/posix_pthread
west twister -p ${platform} -T tests/cpp_pthread

# Zephyr
# west twister -p ${platform} -T tests/subsys/portability/posix/common

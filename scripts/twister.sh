#!/bin/bash

sdir=`dirname $(readlink -f $0)`
rootdir=`dirname $sdir`

source ${sdir}/setenv-run.sh

# platform="qemu_x86_64/atom"
platform="mps2/an385"
# platform="qemu_cortex_r5/zynqmp_rpu"

which qemu

# west twister -p ${platform} -s pthread.dynamic_stack -T tests/pthread

west twister -p ${platform} -T tests
# west twister -p ${platform} -T tests/pthread
# west twister -p ${platform} -T tests/cpp/
# west twister -p ${platform} -T tests/cpp/concurrency
# west twister -p ${platform} -T tests/cpp/basic

#west build -p -t run -b ${platform} tests/cpp/basic -T cpp.basic.dynamic_stack.tls
#west build -p -t debugserver_qemu -b ${platform} tests/cpp/basic -T cpp.basic.dynamic_stack.tls


#!/bin/bash

sdir=`dirname $(readlink -f $0)`
rootdir=`dirname $sdir`

source ${sdir}/setenv-run.sh

# SMP
#platform="qemu_x86_64/atom"
#platform="qemu_cortex_a53/qemu_cortex_a53/smp"
#platform="qemu_riscv64/qemu_virt_riscv64/smp"
#platform="qemu_riscv32/qemu_virt_riscv32/smp"

smp_platforms="-p qemu_x86_64/atom -p qemu_cortex_a53/qemu_cortex_a53/smp -p qemu_riscv64/qemu_virt_riscv64/smp -p qemu_riscv32/qemu_virt_riscv32/smp"

# Uni
#platform="qemu_x86/atom"
platform="mps2/an385"
#platform="qemu_cortex_r5/zynqmp_rpu"
#platform="qemu_riscv32e/qemu_virt_riscv32e"

uni_platforms="-p qemu_x86/atom -p mps2/an385 -p qemu_riscv32e/qemu_virt_riscv32e"
all_platforms="${uni_platforms} ${smp_platforms}"
few_platforms="-p qemu_x86/atom -p mps2/an385 -p qemu_riscv32e/qemu_virt_riscv32e -p qemu_x86_64/atom -p qemu_riscv64/qemu_virt_riscv64/smp"

do_run() {
	which qemu

	# west twister -p ${platform} -s pthread.dynamic_stack -T tests/pthread

	west twister -p ${platform} -T tests
	# west twister -p ${platform} -T tests/pthread
	# west twister -p ${platform} -T tests/cpp/
	# west twister -p ${platform} -T tests/cpp/concurrency
	# west twister -p ${platform} -T tests/cpp/basic

	# west build -p -t run -b ${platform} tests/pthread -T pthread.dynamic_stack.tls

	#west build -p -t run -b ${platform} tests/cpp/basic -T cpp.basic.dynamic_stack.tls
	#west build -p -t debugserver_qemu -b ${platform} tests/cpp/basic -T cpp.basic.dynamic_stack.tls
}

do_run 2>&1 | tee twister.log

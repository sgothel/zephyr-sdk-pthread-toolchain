#!/bin/bash

sdir=`dirname $(readlink -f $0)`
rootdir=`dirname $sdir`

source ${sdir}/setenv-run.sh

#
# Manually resolving twister to silently drop platforms
# when passing multiple platforms.
# See https://github.com/zephyrproject-rtos/zephyr/issues/113359
#
twister_for_all() {
	testsuite_root=$1
	shift 1
	for platform in "$@" ; do
		platform2=$(echo "${platform}" | sed 's/\//_/g')
		odir="twister-out.${platform2}"
		if [ -e "${odir}" ] ; then
			echo "Removing ${odir}"
			rm -rf "${odir}"
		fi
		echo "west twister -O "${odir}" -p ${platform} -T ${testsuite_root}"
		west twister -O "${odir}" -p ${platform} -T ${testsuite_root}
	done
}

# smp
ok_platforms="-p qemu_cortex_a53/qemu_cortex_a53/smp"
ok_platforms="${ok_platforms} -p qemu_riscv64/qemu_virt_riscv64/smp"
ok_platforms="${ok_platforms} -p qemu_x86_64/atom"
# uni
ok_platforms="${ok_platforms} -p mps2/an385"
ok_platforms="${ok_platforms} -p qemu_cortex_a53"
ok_platforms="${ok_platforms} -p qemu_riscv64/qemu_virt_riscv64"
ok_platforms="${ok_platforms} -p qemu_riscv32/qemu_virt_riscv32"
ok_platforms="${ok_platforms} -p qemu_x86/atom"

ok_platforms2=$(echo ${ok_platforms} | sed 's/-p //g')

# SMP
#platform="qemu_cortex_a53/qemu_cortex_a53/smp"
platform="qemu_riscv64/qemu_virt_riscv64/smp"
#platform="qemu_riscv32/qemu_virt_riscv32/smp"
#platform="qemu_x86_64/atom"

smp_platforms="-p qemu_x86_64/atom -p qemu_cortex_a53/qemu_cortex_a53/smp -p qemu_riscv64/qemu_virt_riscv64/smp -p qemu_riscv32/qemu_virt_riscv32/smp"

# Uni
#platform="mps2/an385"
#platform="qemu_cortex_a53"
#platform="qemu_cortex_r5/zynqmp_rpu"
#platform="qemu_riscv64/qemu_virt_riscv64"
#platform="qemu_riscv32/qemu_virt_riscv32"
#platform="qemu_riscv32e/qemu_virt_riscv32e"
#platform="qemu_x86/atom"

uni_platforms="-p qemu_x86/atom -p mps2/an385 -p qemu_riscv32e/qemu_virt_riscv32e"
all_platforms="${uni_platforms} ${smp_platforms}"
few_platforms="-p qemu_x86/atom -p mps2/an385 -p qemu_riscv32e/qemu_virt_riscv32e -p qemu_x86_64/atom -p qemu_riscv64/qemu_virt_riscv64/smp"

do_run() {
	which qemu

	twister_for_all tests ${ok_platforms2}
	# west twister ${ok_platforms} -T tests
	# west twister -p ${platform} -T tests

	# west twister -p ${platform} -T tests/pthread
	# west twister -p ${platform} -T tests/cpp/

	#twister_for_all tests/cpp/concurrency ${ok_platforms2}
	#west twister ${ok_platforms} -T tests/cpp/concurrency
	#west twister -p ${platform} -T tests/cpp/concurrency

	#west twister -p ${platform} -T tests/cpp/basic

	# west build -p -t run -b ${platform} tests/pthread -T pthread.dynamic_stack.tls

	#west build -p -t run -b ${platform} tests/cpp/basic -T cpp.basic.dynamic_stack.tls
	#west build -p -t debugserver_qemu -b ${platform} tests/cpp/basic -T cpp.basic.dynamic_stack.tls

	#west build -p -t run -b ${platform} tests/cpp/basic
	#west build -p -t debugserver -b ${platform} tests/cpp/basic

	#west build -p -t run -b ${platform} tests/cpp/concurrency
	#west build -t debugserver -b ${platform} tests/cpp/concurrency
}

do_run 2>&1 | tee twister.log
grep "failed:" twister.log


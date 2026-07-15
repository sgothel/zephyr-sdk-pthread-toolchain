#!/bin/bash

sdir=`dirname $(readlink -f $0)`
rootdir=`dirname $sdir`

source ${sdir}/setenv-run.sh

# smp
ok_smp_platforms="-p qemu_cortex_a53/qemu_cortex_a53/smp"
ok_smp_platforms="${ok_smp_platforms} -p qemu_riscv64/qemu_virt_riscv64/smp"
ok_smp_platforms="${ok_smp_platforms} -p qemu_x86_64/atom"
# uni
ok_uni_platforms="${ok_uni_platforms} -p mps2/an385"
ok_uni_platforms="${ok_uni_platforms} -p qemu_cortex_a53"
ok_uni_platforms="${ok_uni_platforms} -p qemu_riscv64/qemu_virt_riscv64"
ok_uni_platforms="${ok_uni_platforms} -p qemu_riscv32/qemu_virt_riscv32"
ok_uni_platforms="${ok_uni_platforms} -p qemu_x86/atom"

ok_platforms="${ok_uni_platforms} ${ok_smp_platforms}"

# SMP
#platform="qemu_cortex_a53/qemu_cortex_a53/smp"
#platform="qemu_riscv64/qemu_virt_riscv64/smp"
#platform="qemu_riscv32/qemu_virt_riscv32/smp"
platform="qemu_x86_64/atom"

all_smp_platforms="-p qemu_x86_64/atom -p qemu_cortex_a53/qemu_cortex_a53/smp -p qemu_riscv64/qemu_virt_riscv64/smp -p qemu_riscv32/qemu_virt_riscv32/smp"

# Uni
#platform="mps2/an385"
#platform="qemu_cortex_a53"
#platform="qemu_cortex_r5/zynqmp_rpu"
#platform="qemu_riscv64/qemu_virt_riscv64"
#platform="qemu_riscv32/qemu_virt_riscv32"
#platform="qemu_riscv32e/qemu_virt_riscv32e"
#platform="qemu_x86/atom"

all_uni_platforms="-p qemu_x86/atom -p mps2/an385 -p qemu_riscv32e/qemu_virt_riscv32e"
all_platforms="${all_uni_platforms} ${all_smp_platforms}"
few_platforms="-p qemu_x86/atom -p mps2/an385 -p qemu_riscv32e/qemu_virt_riscv32e -p qemu_x86_64/atom -p qemu_riscv64/qemu_virt_riscv64/smp"

twister_at_once() {
	tag=$1
	shift 1
	testsuite_root=$1
	shift 1
	platforms="$*"
	shift 1
	odir="twister-out.${tag}"
	if [ -e "${odir}" ] ; then
		echo "Removing ${odir}"
		rm -rf "${odir}"
	fi
	echo "west twister -O "${odir}" ${platforms} -T ${testsuite_root} -v"
	west twister -O "${odir}" ${platforms} -T ${testsuite_root} -v
}

twister_for_each() {
	tag=$1
	shift 1
	testsuite_root=$1
	shift 1
	platforms=$(echo "$*" | sed 's/-p //g')
	for platform in ${platforms} ; do
		platform2=$(echo "${platform}" | sed 's/\//_/g')
		odir="twister-out.${tag}.${platform2}"
		if [ -e "${odir}" ] ; then
			echo "Removing ${odir}"
			rm -rf "${odir}"
		fi
		echo "west twister -O "${odir}" -p ${platform} -T ${testsuite_root} -v"
		west twister -O "${odir}" -p ${platform} -T ${testsuite_root} -v
	done
}


do_run() {
	which qemu

	twister_at_once ok_uni tests "${ok_uni_platforms}"
	#twister_for_each smp tests "${ok_smp_platforms}"
	#twister_at_once ok_smp tests "${ok_smp_platforms}"
	#twister_at_once ok_all tests "${ok_platforms}"

	#west twister -p ${platform} -T tests

	# west twister -p ${platform} -T tests/pthread
	# west twister -p ${platform} -T tests/cpp/

	#west twister ${ok_platforms} -T tests/cpp/concurrency -v
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


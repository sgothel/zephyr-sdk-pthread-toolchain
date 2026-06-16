#!/bin/bash

sdir=`dirname $(readlink -f $0)`
rootdir=`dirname $sdir`

#ZEPHYR_SDK_INSTALL_DIR=/home/sven/zephyr-sdk-1.0.1
ZEPHYR_SDK_INSTALL_DIR=/home/sven/zephyr-sdk-1.0.1-mixed

GDB_EXE=${ZEPHYR_SDK_INSTALL_DIR}/gnu/arm-zephyr-eabi/bin/arm-zephyr-eabi-gdb
#GDB_EXE=${ZEPHYR_SDK_INSTALL_DIR}/gnu/x86_64-zephyr-elf/bin/x86_64-zephyr-elf-gdb

# Server @ localhost:1234 qemu
# west build -t debugserver_qemu

cd build/zephyr
echo
echo "GDB:"
echo "  target remote localhost:1234"
echo "  dir /usr/local/projects/OS/zephyrproject-4.4.0/zephyr"
echo
ddd --gdb --debugger "${GDB_EXE} -x ${sdir}/gdb-cmds00.txt zephyr.elf"
#ddd --gdb --debugger "${GDB_EXE} -x ${sdir}/gdb-cmds01.txt zephyr.elf"
#ddd --gdb --debugger "${GDB_EXE} -x ${sdir}/gdb-cmds02.txt zephyr.elf"

# GDB
# target remote localhost:1234
# dir /usr/local/projects/OS/zephyrproject-4.4.0/zephyr

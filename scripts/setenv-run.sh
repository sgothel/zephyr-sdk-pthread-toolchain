#!/bin/bash

# sdir=`dirname $(readlink -f $0)`
# rootdir=`dirname $sdir`
zrootdir=/usr/local/projects/OS/zephyrproject

#python3 -m venv $zrootdir/.venv
source $zrootdir/.venv/bin/activate
source $zrootdir/zephyr/zephyr-env.sh

export ZEPHYR_SDK_INSTALL_DIR=${HOME}/zephyr-sdk-1.0.1-mixed
# export ZEPHYR_SDK_INSTALL_DIR=${HOME}/zephyr-sdk-1.0.1

echo "ZEPHYR_SDK_INSTALL_DIR ${ZEPHYR_SDK_INSTALL_DIR}"
echo "ZEPHYR_BASE ${ZEPHYR_BASE}"


#!/bin/bash

unset ZEPHYR_TOOLCHAIN_VARIANT
unset CROSS_COMPILE

#export ZEPHYR_SDK_INSTALL_DIR=${HOME}/zephyr-sdk-1.0.1
export ZEPHYR_SDK_INSTALL_DIR=${HOME}/zephyr-sdk-1.0.1-mixed

rootdir=/usr/local/projects/OS/zephyrproject

source $rootdir/.venv/bin/activate
source $rootdir/zephyr/zephyr-env.sh

echo "ZEPHYR_SDK_INSTALL_DIR ${ZEPHYR_SDK_INSTALL_DIR}"
echo "ZEPHYR_TOOLCHAIN_VARIANT ${ZEPHYR_TOOLCHAIN_VARIANT}"
echo "CROSS_COMPILE ${CROSS_COMPILE}"
echo "ZEPHYR_BASE ${ZEPHYR_BASE}"

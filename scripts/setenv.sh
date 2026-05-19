#!/bin/sh

export ZEPHYR_SDK_INSTALL_DIR=${HOME}/zephyr-sdk-1.0.1-mixed
export ZEPHYR_SDK_WORKSPACE=/usr/local/projects/OS/zephyrproject-sdk-ng/sdk-ng
export ZEPHYR_BASE=/usr/local/projects/OS/zephyrproject/zephyr

export PATH=/usr/local/bin:${PATH}
export CT_PREFIX=${HOME}/crosstool-ng
export CT_LOCAL_TARBALLS_DIR=${CT_PREFIX}/src
mkdir -p ${CT_LOCAL_TARBALLS_DIR}

echo "ZEPHYR_SDK_INSTALL_DIR ${ZEPHYR_SDK_INSTALL_DIR}"
echo "ZEPHYR_SDK_WORKSPACE ${ZEPHYR_SDK_WORKSPACE}"
echo "ZEPHYR_BASE ${ZEPHYR_BASE}"

echo "PATH ${PATH}"
echo "CT_PREFIX ${CT_PREFIX}"
echo "CT_LOCAL_TARBALLS_DIR ${CT_LOCAL_TARBALLS_DIR}"

ulimit -Sn 2048

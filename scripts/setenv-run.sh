#!/bin/bash

sdir=`dirname $(readlink -f ${BASH_SOURCE[0]})`
rootdir=`dirname $sdir`

source ${sdir}/setenv.sh
# export ZEPHYR_BASE=/usr/local/projects/OS/zephyrproject/zephyr

zrootdir=`dirname ${ZEPHYR_BASE}`

#python3 -m venv $zrootdir/.venv
source $zrootdir/.venv/bin/activate
source $zrootdir/zephyr/zephyr-env.sh

# export ZEPHYR_SDK_INSTALL_DIR=${HOME}/zephyr-sdk-1.0.1-mixed


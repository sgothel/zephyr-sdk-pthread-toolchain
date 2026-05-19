#!/bin/bash

sdir=`dirname $(readlink -f $0)`
rootdir=`dirname $sdir`

rm -rf ${rootdir}/temp

#
# copy includes
mkdir -p ${rootdir}/temp/include/inject/zephyr
mkdir -p ${rootdir}/temp/include/inject/arm-zephyr-eabi
cp -a ${rootdir}/include/inject/zephyr/* ${rootdir}/temp/include/inject/zephyr/
cp ${ZEPHYR_SDK_INSTALL_DIR}/gnu/arm-zephyr-eabi/arm-zephyr-eabi/include/picolibc.h ${rootdir}/temp/include/inject/arm-zephyr-eabi/

#

#
# config
#
rm -f .config
cat sections/common.config sections/arm-zephyr-eabi.config > .config
ct-ng upgradeconfig


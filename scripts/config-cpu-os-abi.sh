#!/bin/bash

sdir=`dirname $(readlink -f $0)`
rootdir=`dirname $sdir`

source ${sdir}/setenv.sh
source ${sdir}/functions.sh

set_target_triple $(basename $0 .sh) "config"

check_file ${ZEPHYR_SDK_INSTALL_DIR}/gnu/${target_triple}/${target_triple}/include/picolibc.h
check_file sections/${target_triple}.config

rm -rf ${rootdir}/temp

#
# copy includes
mkdir -p ${rootdir}/temp/include/inject/zephyr
mkdir -p ${rootdir}/temp/include/inject/${target_triple}
cp -a ${rootdir}/include/inject/zephyr/* ${rootdir}/temp/include/inject/zephyr/
cp ${ZEPHYR_SDK_INSTALL_DIR}/gnu/${target_triple}/${target_triple}/include/picolibc.h ${rootdir}/temp/include/inject/${target_triple}/

#

#
# config
#
cd ${rootdir}
rm -f .config
cat sections/common.config sections/${target_triple}.config > .config
ct-ng upgradeconfig


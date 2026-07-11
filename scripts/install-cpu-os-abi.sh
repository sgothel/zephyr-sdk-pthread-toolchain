#!/bin/bash

sdir=`dirname $(readlink -f $0)`
rootdir=`dirname $sdir`

source ${sdir}/setenv.sh
source ${sdir}/functions.sh

set_target_triple $(basename $0 .sh) "install"

check_dir ${CT_PREFIX}/${target_triple}

move_to_old ${ZEPHYR_SDK_INSTALL_DIR}/gnu/${target_triple}

echo "Install ${CT_PREFIX}/${target_triple} to ${ZEPHYR_SDK_INSTALL_DIR}/gnu/"
cp -a ${CT_PREFIX}/${target_triple} ${ZEPHYR_SDK_INSTALL_DIR}/gnu/



# Zephyr Installation and Setup

We install multiple versions of Zephyr,
each in its own subdirectory.
- branch `main`
  - directory `/usr/local/projects/OS/zephyrproject`
- branch `v4.4-branch`
  - directory `/usr/local/projects/OS/zephyrproject-4.4.0`

See [Zephyr Getting Started](https://docs.zephyrproject.org/latest/develop/getting_started/index.html)
and [Managing Multiple Versions of the Zephyr RTOS](https://www.zephyrproject.org/managing-multiple-versions-of-the-zephyr-rtos/).

### Debian/Ubuntu Prerequisites
See [Zephyr Linux Installation](https://docs.zephyrproject.org/latest/develop/getting_started/installation_linux.html#installation-linux).

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.sh}
apt-get install --no-install-recommends git cmake ninja-build gperf \
  ccache dfu-util device-tree-compiler wget \
  python3-dev python3-pip python3-setuptools python3-tk python3-wheel xz-utils file \
  make gcc gcc-multilib g++-multilib libsdl2-dev libmagic1
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

## Single Zephyr Version Installation

We create one folder for each version, e.g. `zephyrproject` for the `main` branch
and `zephyrproject-4.4.0` for the `v4.4-branch`.

Each folder acts as the root directory for each Zephyr installation.

Each Zephyr installation denotes its own `ZEPHYR_BASE`, e.g. `zephyrproject-4.4.0/zephyr`.

All scripts resides in each Zephyr installation's `scripts` subdirectory, e.g. `zephyrproject-4.4.0/scripts`.

### Python Environment Setup
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.sh}
#!/bin/bash

sdir=`dirname $(readlink -f $0)`
rootdir=`dirname $sdir`

python3 -m venv $rootdir/.venv
#source $rootdir/.venv/bin/activate
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

### Zephyr Setup
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.sh}
#!/bin/bash

sdir=`dirname $(readlink -f $0)`
rootdir=`dirname $sdir`

pip install west

# 4.4.0 Version
west init --mr v4.4-branch $rootdir
# Latest Version, main branch
# west init $rootdir

cd $rootdir
west update

west zephyr-export
west packages pip --install

cd $rootdir/zephyr
#west sdk install --help
# Since 4.4.0
west sdk install --gnu-toolchains arm-zephyr-eabi
# Fo 4.3.0
# west sdk install --toolchains arm-zephyr-eabi

# ~/zephyr-sdk-1.0.1/
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

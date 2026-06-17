# Zephyr SDK `pthread` Toolchain

## Goals

Goals is to enable `posix threads` via the build-in `pthreads` [concurrency implementation](https://gcc.gnu.org/onlinedocs/libstdc++/manual/using_concurrency.html) in the [GCC](https://gcc.gnu.org/) and its [GNU C++ Library](https://gcc.gnu.org/onlinedocs/libstdc++/manual/) toolchain.

This will enable full [POSIX pthread](https://pubs.opengroup.org/onlinepubs/7908799/xsh/pthread.h.html) support throughout
[Zephyr RTOS](https://www.zephyrproject.org/), `GCC` and its `libstdc++` including [C++ Concurrency Support (thread)](https://en.cppreference.com/cpp/thread)
and hence widening Zephyr's C++ compatibility.

Since both toolchain components already support the `pthread` implementation across multiple platforms,
we can achieve this goal without code change in the toolchains.

Zephyr already has optional `pthread` and POSIX support with the following `Kconfig` setup

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Required KConfig (no change)
CONFIG_MULTITHREADING=y
CONFIG_POSIX_API=y
CONFIG_POSIX_THREADS=y

# Recommended General KConfig (additional)
CONFIG_THREAD_LOCAL_STORAGE=y
CONFIG_CURRENT_THREAD_USE_TLS=y
CONFIG_THREAD_NAME=y
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

## Solution
### Zephyr `pthread` patches
Only a few minimal Zephyr PRs are required to make its `pthread` implementation functional for this purpose:
- [pthread: Support handling Zephyr native threads #109291](https://github.com/zephyrproject-rtos/zephyr/pull/109291)
- [posix: cond: Allow statically initialized cond to pthread_cond_broadcast #111547](https://github.com/zephyrproject-rtos/zephyr/pull/111547)

### Injected Zephyr `pthread.h` header
To build the toolchain w/ the Zephyr `pthread` header, the [amalgamated pthread.h](include/inject/zephyr/pthread.h)
is injected into the process. This header is an all-in-one merged, architecture agnostic version.

### Toolchain `GCC` and `libstc++` Configuration
The configuration flags `--enable-tls --enable-threads=posix --enable-libstdcxx-threads` have to be enabled.

See [Zephyr SDK Toolchain Build Details](doc/Zephyr-SDK-Build.md) for details.

## Previous Work
[OS Issue 25569](https://github.com/zephyrproject-rtos/zephyr/issues/25569)
used `C11 threads` for GCC's `gthread` implementation and required
a patched `GCC` and `crosstool-ng`.

See [Zephyr POSIX and C++ Support](doc/Zephyr-PosixAndCppSupport.md) for more details.

## Further documentation
- [Zephyr Setup](doc/Zephyr-Setup.md).
- [Zephyr POSIX and C++ Support](doc/Zephyr-PosixAndCppSupport.md)
- [Zephyr SDK Toolchain Build Details](doc/Zephyr-SDK-Build.md)

## Procedure

### Related Directories

`scripts/setenv.sh` sets up environment variables as expected in this description.
If diverting, please change `scripts/setenv.sh` and throughout your build procedure.

| Variable                   | Path                                                   |
| -------------------------- | ------------------------------------------------------ |
| `PATH`                     | `/usr/local/bin:${PATH}`                               |
| `CT_PREFIX`                | `${HOME}/crosstool-ng`                                 |
| `CT_LOCAL_TARBALLS_DIR`    | `${CT_PREFIX}/src`                                     |
| `ZEPHYR_SDK_INSTALL_DIR`   | `${HOME}/zephyr-sdk-1.0.1-mixed`                       |
| `ZEPHYR_SDK_ROOT`          | `/usr/local/projects/OS/zephyrproject-sdk-ng`          |
| `ZEPHYR_SDK_WORKSPACE`     | `${ZEPHYR_SDK_ROOT}/sdk-ng`                            |
| `ZEPHYR_BASE`              | `/usr/local/projects/OS/zephyrproject/zephyr`          |

`scripts/setenv.sh` also creates `CT_LOCAL_TARBALLS_DIR`, if not existing.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.sh}
mkdir -p ${CT_LOCAL_TARBALLS_DIR}
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

### Environment Variable Setup
Setting up directories as described above

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.sh}
. ./scripts/setenv.sh
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

### SDK Source Repository Setup
Cloning the git repository as follows

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.sh}
mkdir -p ${ZEPHYR_SDK_ROOT}
cd ${ZEPHYR_SDK_ROOT}
git clone https://github.com/zephyrproject-rtos/sdk-ng
cd ${ZEPHYR_SDK_WORKSPACE}
git checkout -b zephyr-threading-01 8191f3ad25827a79bf15dd085823e87d27127d85
git submodule update --init
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

See details in [Zephyr SDK Toolchain Build Details](doc/Zephyr-SDK-Build.md).

### SDK Cloning
Note, this procedure uses a cloned SDK 1.0.1 version at `${HOME}/zephyr-sdk-1.0.1`:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.sh}
cp -a ${HOME}/zephyr-sdk-1.0.1 ${HOME}/zephyr-sdk-1.0.1-mixed
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

For other locations, please adjust `scripts/setenv.sh`.

### Install crosstools-ng 
I have used [crosstools-ng](https://crosstool-ng.github.io/) version 1.28.0,
build from its [git repo](https://github.com/crosstool-ng/crosstool-ng) as follows.

To install the Debian/Ubuntu prerequisites as described below,
you may also use [scripts/install-prerequisites.sh](scripts/install-prerequisites.sh).

Install `crosstool-ng` under `/usr/local`.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.sh}
# Debian or Ubuntu
sudo apt-get install git build-essential autotools-dev automake autoconf g++ gcc libc-dev libpthread-stubs0-dev \
                     cmake cmake-extras extra-cmake-modules ccache \
                     texinfo gawk libtool-bin flex bison libncurses-dev \
                     lzip curl meson ninja-build device-tree-compiler \
                     help2man python3-dev

git clone https://github.com/crosstool-ng/crosstool-ng
cd crosstool-ng
git checkout -b b_1.28.0 crosstool-ng-1.28.0
./bootstrap

./configure --prefix=/usr/local
make
sudo make install
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

### Toolchain build w/ crosstools-ng
Brief description of building the toolchain and installing it over the pre-existing SDK.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.sh}
. scripts/setenv.sh
./scripts/config-arm-zephyr-eabi.sh
ct-ng build

rm -rf ${ZEPHYR_SDK_INSTALL_DIR}/gnu/arm-zephyr-eabi.old
mv ${ZEPHYR_SDK_INSTALL_DIR}/gnu/arm-zephyr-eabi ${ZEPHYR_SDK_INSTALL_DIR}/gnu/arm-zephyr-eabi.old
cp -a ${CT_PREFIX}/arm-zephyr-eabi ${ZEPHYR_SDK_INSTALL_DIR}/gnu/
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


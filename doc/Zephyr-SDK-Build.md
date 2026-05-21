# Zephyr SDK Toolchain Build Details

See [crosstools-ng build](../README.md) for a brief build description.

Resources
- [Zephyr SDK Doc](https://docs.zephyrproject.org/latest/develop/toolchains/zephyr_sdk.html)
- [Zephyr SDK Repo](https://github.com/zephyrproject-rtos/sdk-ng)

## Debian/Ubuntu Prerequisites
See [Zephyr Linux Installation](https://docs.zephyrproject.org/latest/develop/getting_started/installation_linux.html#installation-linux).

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.sh}
apt-get install --no-install-recommends git cmake ninja-build gperf \
  ccache dfu-util device-tree-compiler wget \
  python3-dev python3-pip python3-setuptools python3-tk python3-wheel xz-utils file \
  make gcc gcc-multilib g++-multilib libsdl2-dev libmagic1
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

## Clone git repository

We assume `ZEPHYR_SDK_ROOT` was set to `/usr/local/projects/OS/zephyrproject-sdk-ng`
and `ZEPHYR_SDK_WORKSPACE` was set to `/usr/local/projects/OS/zephyrproject-sdk-ng/sdk-ng`.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.sh}
mkdir -p ${ZEPHYR_SDK_ROOT}
cd ${ZEPHYR_SDK_ROOT}
git clone https://github.com/zephyrproject-rtos/sdk-ng
cd ${ZEPHYR_SDK_WORKSPACE}
git submodule update --init
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

## GCC C++ Configuration

### GCC Configuration
[Installing GCC: Configuration](https://gcc.gnu.org/install/configure.html)

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Default autoselect best or only available, may be `single`
* --enable-threads
# model {single, posix, lynx, aix, vxworks, win32, ...}
* --enable-threads=<model>

* --enable-tls
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

### GNU C++ Library Configuration
[Configuration](https://gcc.gnu.org/onlinedocs/libstdc++/manual/configure.html)
[Macros](https://gcc.gnu.org/onlinedocs/libstdc++/manual/using_macros.html)
[Exceptions](https://gcc.gnu.org/onlinedocs/libstdc++/manual/using_exceptions.html)
[Concurrency](https://gcc.gnu.org/onlinedocs/libstdc++/manual/using_concurrency.html)

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# lock policy that controls how shared_ptr reference counting is synchronized.
# option { auto, atomic, mutex }
* --with-libstdcxx-lock-policy=option

# Use static buffer (pool) instead of dynamic malloc
* --enable-libstdcxx-static-eh-pool

# Each object of size 6 * sizeof(void*)
* --with-libstdcxx-eh-pool-obj-count=24
*
* --enable-libstdcxx-backtrace=yes

* --enable-libstdcxx-threads

# link-type checks for the availability of the clock_gettime clocks, used in the implementation of [time.clock],
# and of the nanosleep and sched_yield functions,
* --enable-libstdcxx-time=auto

# Build separate debug libraries in addition to what is normally built (CXXFLAGS='-g3 -O0 -fno-inline')
* --enable-libstdcxx-debug
* --enable-libstdcxx-debug-flags=FLAGS

#
* --enable-cxx-flags=FLAGS
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

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
- [posix: cond: Allow statically initialized cond to pthread_cond_broadcast #111547](https://github.com/zephyrproject-rtos/zephyr/pull/111547)
  - Branch [zephyr-pthread_cond_broadcast](https://github.com/sgothel/zephyr/tree/zephyr-pthread_cond_broadcast)
  - Merged w/ Zephyr upstream
- [pthread: Support handling Zephyr native threads #109291](https://github.com/zephyrproject-rtos/zephyr/pull/109291)
  - Branch [zephyr-threading-01](https://github.com/sgothel/zephyr/tree/zephyr-threading-01)
  - Includes upstream merged branch [zephyr-pthread_cond_broadcast](https://github.com/sgothel/zephyr/tree/zephyr-pthread_cond_broadcast)
- [pthread: Unwind & execute all cleanup handler where required #112736](https://github.com/zephyrproject-rtos/zephyr/pull/112736)
  - Branch [zephyr-pthread_cleanup](https://github.com/sgothel/zephyr/tree/zephyr-pthread_cleanup)
  - Is on top of branch [zephyr-threading-01](https://github.com/sgothel/zephyr/tree/zephyr-threading-01), i.e. included
- [posix: pthread: Remove all key associated references in thread-pool #112960](https://github.com/zephyrproject-rtos/zephyr/pull/112960)
  - Branch [zephyr-pthread_specific](https://github.com/sgothel/zephyr/tree/zephyr-pthread_specific)
  - Is on top of branch [zephyr-pthread_cleanup](https://github.com/sgothel/zephyr/tree/zephyr-pthread_cleanup)
- [posix: pthread: Fix pthread_once by adding wait-state for concurrent thread #113166](https://github.com/zephyrproject-rtos/zephyr/pull/113166)
  - Branch [zephyr-pthread_once](https://github.com/sgothel/zephyr/tree/zephyr-pthread_once)
  - Is on top of branch [zephyr-pthread_specific](https://github.com/sgothel/zephyr/tree/zephyr-pthread_specific)

I provide an [almagated branch](https://github.com/sgothel/zephyr/tree/zephyr-threading-almagated) including
above PR branches for testing convenience.

Older almagated branches have a two-digit-suffix, the lowed the suffix the older the branch.
These are added for testing purposes (reproduction, etc).

### Injected Zephyr `pthread.h` header
To build the toolchain w/ the Zephyr `pthread` header, the [amalgamated pthread.h](include/inject/zephyr/pthread.h)
is injected into the process. This header is an all-in-one merged, architecture agnostic version.

### Toolchain `GCC` and `libstc++` Configuration
The configuration flags `--enable-tls --enable-threads=posix --enable-libstdcxx-threads` have to be enabled.

See [Zephyr SDK Toolchain Build Details](doc/Zephyr-SDK-Build.md) for details.

## Testing
First build and install the target toolchain as an overlay
to the existing SDK 1.0.1 as described in `Procedure` below.

This repository contains specific goal oriented twister ZTests
for `pthread` and C++ functionality.

Assuming `scripts/setenv.sh` has been setup properly,
see `Procedure below`, you can simply launch

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.sh}
. scripts/setenv-run.sh

west twister -p mps2/an385 -T tests
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Alternatively, you can edit `./scripts/twister.sh` and enable the desired test platform(s)
and/or tests - and run it

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.sh}
. ./scripts/twister.sh
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

## Results
Working compiler-toolchain and target-platform,
passing all our twister ZTests:
- `arm-zephyr-eabi`
  - `mps2/an385`
- `aarch64-zephyr-elf`
  - `qemu_cortex_a53`
  - `qemu_cortex_a53/qemu_cortex_a53/smp`
- `riscv64-zephyr-elf`
  - `qemu_riscv64/qemu_virt_riscv64`
  - `qemu_riscv64/qemu_virt_riscv64/smp`
  - `qemu_riscv32/qemu_virt_riscv32`

Having issues still
- `riscv64-zephyr-elf`
  - `qemu_riscv32/qemu_virt_riscv32/smp`
    - `tests/cpp/basic/src/alloc_new.cpp`
- `x86_64-zephyr-elf`
  - `qemu_x86_64/atom`
    - C++ exceptions
  - `qemu_x86/atom`
    - C++ exceptions

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
Brief description of building the toolchain and installing it over the pre-existing SDK
for tested target-platforms.

#### `arm-zephyr-eabi`
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.sh}
./scripts/config-arm-zephyr-eabi.sh
ct-ng build

./scripts/install-arm-zephyr-eabi.sh
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#### `aarch64-zephyr-elf`
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.sh}
./scripts/config-aarch64-zephyr-elf.sh
ct-ng build

./scripts/install-aarch64-zephyr-elf.sh
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#### `riscv64-zephyr-elf`
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.sh}
./scripts/config-riscv64-zephyr-elf.sh
ct-ng build

./scripts/install-riscv64-zephyr-elf.sh
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#### `x86_64-zephyr-elf`
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.sh}
./scripts/config-x86_64-zephyr-elf.sh
ct-ng build

./scripts/install-x86_64-zephyr-elf.sh
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


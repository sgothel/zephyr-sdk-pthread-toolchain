# Zephyr POSIX and C++ Support

## Zephyr SDK
The SDK contains
- Compiler toolchain for each target
- C++ Standard Library implementation, GCC and optionally LLVM
- Miscellaneous tools

Resources
- [Zephyr SDK Doc](https://docs.zephyrproject.org/latest/develop/toolchains/zephyr_sdk.html)
- [Zephyr SDK Repo](https://github.com/zephyrproject-rtos/sdk-ng)

## Test Emulation

We test for an ARM 32-bit board, i.e. `ARM Cortex-M3 CPU`.

[Available emulation targets](https://docs.zephyrproject.org/latest/hardware/arch/arm_cortex_m.html#qemu)
matching our requirements are
- `qemu_cortex_m3` (see below)
- `mps2/an385` (see below)

### `qemu_cortex_m3`
We started with the [`qemu_cortex_m3`](https://docs.zephyrproject.org/latest/boards/qemu/cortex_m3/doc/index.html)
target board.
However, the flash limitation of 256 kB is too restrictive
with debug symbols and instrumentation enabled.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.sh}
# qemu_cortex_m3 is limited to 256 kB Flash, no MPU
west build -p always -b qemu_cortex_m3 .
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

### `mps2/an385`

The [Board V2M MPS2 Family](https://docs.zephyrproject.org/latest/boards/arm/mps2/doc/index.html)
supports [mps2/an385](https://docs.zephyrproject.org/latest/boards/arm/mps2/doc/mps2_armv7m.html#mps2-armv7m-board)
and exceeds our requirement, a good fit for development
- mps2 Armv7-m based board targets supported in Zephyr
- ARM Cortex-M3 CPU
- MPU suport
- 4MB Flash

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.sh}
west build -p always -b mps2/an385 .
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

## C Library

### `Picolib` default
- [Picolib Doc](https://docs.zephyrproject.org/latest/develop/languages/c/picolibc.html)
- `CONFIG_PICOLIBC` (default)

### `Newlib` support dropped
- [Newlib Doc](https://docs.zephyrproject.org/latest/develop/languages/c/newlib.html)
- [SDK 1.0.1 has removed newlib](https://github.com/zephyrproject-rtos/sdk-ng/issues/896)
- `CONFIG_NEWLIB_LIBC_SUPPORTED` is disabled

## POSIX Threading

### POSIX pthread
[pthread](https://pubs.opengroup.org/onlinepubs/9699919799/) is supported
and enabled via `CONFIG_POSIX_THREADS`.

Zephyr implements pthread via its kernel threading API,
see header and sources
- [${ZEPHYR\_BASE}/include/zephyr/posix/pthread.h](https://github.com/zephyrproject-rtos/zephyr/blob/main/include/zephyr/posix/pthread.h)
- [${ZEPHYR\_BASE}/subsys/portability/posix/options/pthread.c](https://github.com/zephyrproject-rtos/zephyr/blob/main/subsys/portability/posix/options/pthread.c)

### POSIX C11 threads
[C11 threads](https://en.cppreference.com/c/header/threads) are supported
and enabled via `CONFIG_COMMON_LIBC_THRD`.

Zephyr implements C11 threads via pthread, see source code
- [${ZEPHYR\_BASE}/lib/libc/common/source/thrd/thrd.c](https://github.com/zephyrproject-rtos/zephyr/blob/main/lib/libc/common/source/thrd/thrd.c)

### Discussions and Issues
- [lib: posix: dynamic stack support for pthread_create#44727](https://github.com/zephyrproject-rtos/zephyr/pull/44727)
  - PR by Chris Friedt (cfriedt) to support automatic thread stack allocation for [pthread\_create()](https://pubs.opengroup.org/onlinepubs/9699919799/functions/pthread_create.html), which is required as part of IEEE Std. 1003.2017 (and all earlier versions).

## C++ Language Support
- [C++ Language Support Doc](https://docs.zephyrproject.org/latest/develop/languages/cpp/index.html)
- [Open `area: C++` issues](https://github.com/zephyrproject-rtos/zephyr/issues?q=state%3Aopen%20label%3A%22area%3A%20C%2B%2B%22)
  - [Closed  `area: C++` issues](https://github.com/zephyrproject-rtos/zephyr/issues?q=state%3Aclosed%20label%3A%22area%3A%20C%2B%2B%22)
- [C++ Roadmap #45785](https://github.com/zephyrproject-rtos/zephyr/issues/45785)
- [C++ support in Zephyr #31281](https://github.com/zephyrproject-rtos/zephyr/issues/31281)
- [Support for std::thread and std::this_thread #25569](https://github.com/zephyrproject-rtos/zephyr/issues/25569)
  - SDK `newlib` [improve support for c++ threading and synchronization#735](https://github.com/zephyrproject-rtos/sdk-ng/pull/735) (reverted)
    - This area is also addressing our C++ stdlib thread-safety issues, **see below**
  - Closed [cpp: thread: support for std::thread, std::mutex, std::condition_variable, etc #43729](https://github.com/zephyrproject-rtos/zephyr/pull/43729)
  - ..

### Historical
- [C++ 11 Support #4028](https://github.com/zephyrproject-rtos/zephyr/issues/4028)
- [Tracking Issue for C++ Support as of release 2.1 #18554](https://github.com/zephyrproject-rtos/zephyr/issues/18554)

## Thread-Safety w/ C and C++
### Kconfig
- `CONFIG_COMMON_LIBC_THRD` C11 threads
- `CONFIG_COMMON_LIBC_ASCTIME_R`Thread-safe version of asctime()
- `CONFIG_COMMON_LIBC_CTIME_R` Thread-safe version of ctime()
- `CONFIG_POSIX_C_LANG_SUPPORT_R` Thread-Safe General ISO C Library
- `CONFIG_POSIX_FILE_SYSTEM_R` Thread-Safe File System
- `CONFIG_POSIX_THREAD_SAFE_FUNCTIONS` POSIX thread-safe functions

### Discussions and Issues
- [Thread-safety of C-Library and C++ runtime #105427](https://github.com/zephyrproject-rtos/zephyr/issues/105427)
  - Discussion by Daniel Jerolm about issues w/
    - Exceptions thrown from different threads leads to UB, not thread-safe? *see below*
    - `asctime` returned pointer not thread-safe? *resolved*
    - `strtok` internal state not thread-safe? *resolved*

Tests to disclose this behavior
- Exceptions thrown and caught by 2 threads, corrupts memory of the exceptions
- `std::shared_ptr` access is not synchronized?
- `std::atomic` is not synchronized?

### GCC Toolchain Built Not Thread-Safe
In SDK 1.0.1 it is build
- `--with-default-libc=picolibc`
- Languages C, C++
- `Thread model: single`

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.sh}
> ~/zephyr-sdk-1.0.1/gnu/arm-zephyr-eabi/bin/arm-zephyr-eabi-gcc -v

Using built-in specs.
COLLECT_GCC=/home/sven/zephyr-sdk-1.0.1/gnu/arm-zephyr-eabi/bin/arm-zephyr-eabi-gcc
COLLECT_LTO_WRAPPER=/home/sven/zephyr-sdk-1.0.1/gnu/arm-zephyr-eabi/bin/../libexec/gcc/arm-zephyr-eabi/14.3.0/lto-wrapper
Target: arm-zephyr-eabi
Configured with: /__w/_temp/workspace/build/.build/arm-zephyr-eabi/src/gcc/configure --build=x86_64-build_pc-linux-gnu --host=x86_64-build_pc-linux-gnu --target=arm-zephyr-eabi --prefix=/__w/_temp/workspace/output/arm-zephyr-eabi --exec_prefix=/__w/_temp/workspace/output/arm-zephyr-eabi --with-local-prefix=/__w/_temp/workspace/output/arm-zephyr-eabi/arm-zephyr-eabi --with-headers=/__w/_temp/workspace/output/arm-zephyr-eabi/arm-zephyr-eabi/include --with-newlib --enable-threads=no --disable-shared --with-pkgversion='Zephyr SDK 1.0.1' --with-bugurl=https://github.com/zephyrproject-rtos/sdk-ng/issues --enable-__cxa_atexit --disable-libgomp --disable-libmudflap --disable-libmpx --disable-libssp --disable-libquadmath --disable-libquadmath-support --disable-libstdcxx-verbose --with-default-libc=picolibc --enable-cstdio=stdio_pure --with-gmp=/__w/_temp/workspace/build/.build/arm-zephyr-eabi/buildtools --with-mpfr=/__w/_temp/workspace/build/.build/arm-zephyr-eabi/buildtools --with-mpc=/__w/_temp/workspace/build/.build/arm-zephyr-eabi/buildtools --with-isl=/__w/_temp/workspace/build/.build/arm-zephyr-eabi/buildtools --enable-lto --enable-target-optspace --disable-plugin --disable-nls --enable-multiarch --with-multilib-list=rmprofile --enable-multilib-space --enable-languages=c,c++ --with-gnu-ld --with-gnu-as --enable-initfini-array
Thread model: single
Supported LTO compression algorithms: zlib zstd
gcc version 14.3.0 (Zephyr SDK 1.0.1)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This indicates that C++ thread-safety is lacking,
since atomics and general threading instrumentation
uses unsynchronized code.

See `sdk-ng/gcc/libstdc++-v3/include/ext/concurrence.h`

~~~~~~~~~~~~~~
 44   // Available locking policies:
 45   // _S_single    single-threaded code that doesn't need to be locked.
 46   // _S_mutex     multi-threaded code that requires additional support
 47   //              from gthr.h or abstraction layers in concurrence.h.
 48   // _S_atomic    multi-threaded code using atomic operations.
 49   enum _Lock_policy { _S_single, _S_mutex, _S_atomic };
 50
 51   // Compile time constant that indicates prefered locking policy in
 52   // the current configuration.
 53   _GLIBCXX17_INLINE const _Lock_policy __default_lock_policy =
 54 #ifndef __GTHREADS
 55   _S_single;
 56 #elif defined _GLIBCXX_HAVE_ATOMIC_LOCK_POLICY
 57   _S_atomic;
 58 #else
 59   _S_mutex;
 60 #endif
~~~~~~~~~~~~~~

### Enable Thread-Safety in GCC Toolchain

For normal thread-safe behavior, i.e. general multi-threading support
for locking and atomic operation GCC expects the following macros defined
- `__GTHREADS`
- `_GLIBCXX_HAVE_ATOMIC_LOCK_POLICY`

#### Previous work addressing [OS Issue 25569](https://github.com/zephyrproject-rtos/zephyr/issues/25569)
This work has been reverted due to lack of single-threaded compatibility.

Notable: The reasoning for reversion only addresses compatibility with single-thread applications,
which is of no issue for a (C++) multi-threaded target. Here, all required configured features have to be always enabled.

- OS Issue [Support for std::thread and std::this_thread #25569](https://github.com/zephyrproject-rtos/zephyr/issues/25569)
  - SDK PR [improve support for c++ threading and synchronization#735](https://github.com/zephyrproject-rtos/sdk-ng/pull/735)
    - 1st commit [8dee61674c90dbe6ccd534a7de2f1959e3514d6c](https://github.com/zephyrproject-rtos/sdk-ng/commit/8dee61674c90dbe6ccd534a7de2f1959e3514d6c)
  - GCC PR [libgcc: implement gthreads in terms of C11 threads #27](https://github.com/zephyrproject-rtos/gcc/pull/27)
    - Commit 1 [685f45182f4442aff383175c28e8789bbccfceb5](https://github.com/zephyrproject-rtos/gcc/commit/685f45182f4442aff383175c28e8789bbccfceb5)
    - Commit 2 [52dc7f4a2da843d6a3d8524a040bd75e33a75c58](https://github.com/zephyrproject-rtos/gcc/commit/52dc7f4a2da843d6a3d8524a040bd75e33a75c58)
    - Commit 2 [47b9276c90496367ce6337a5751dccb4e5b1a19a](https://github.com/zephyrproject-rtos/gcc/commit/47b9276c90496367ce6337a5751dccb4e5b1a19a)
- SDK Reversion [libgcc: C11 threads-based gthread support cannot link samples/cpp/hello_world out-of-the-box #751](https://github.com/zephyrproject-rtos/sdk-ng/issues/751)
  - SDK PR [Revert "configs: common: enable c++11 thread support via c11 threads" #753](https://github.com/zephyrproject-rtos/sdk-ng/pull/753)
    - Commit [2c75c61acd58fa8bc3a7d2c2e01b7b2a0354b1e0](https://github.com/zephyrproject-rtos/sdk-ng/commit/2c75c61acd58fa8bc3a7d2c2e01b7b2a0354b1e0)
  - GCC PR [Revert "libgcc: gthr: support for --enable-threads=c11" until compatibility issue is resolved #30](https://github.com/zephyrproject-rtos/gcc/pull/30)
    - Commit [a6b368938ee06bef8b8848850a4d6f0300975c57](https://github.com/zephyrproject-rtos/gcc/pull/30/changes/a6b368938ee06bef8b8848850a4d6f0300975c57)
      - Reverts [47b9276c90496367ce6337a5751dccb4e5b1a19a](https://github.com/zephyrproject-rtos/gcc/commit/47b9276c90496367ce6337a5751dccb4e5b1a19a)
    - Commit [0893ef96acb0b979b96feb7510fc4a18766dc17a](https://github.com/zephyrproject-rtos/gcc/pull/30/changes/0893ef96acb0b979b96feb7510fc4a18766dc17a)
      - Reverts [52dc7f4a2da843d6a3d8524a040bd75e33a75c58](https://github.com/zephyrproject-rtos/gcc/commit/52dc7f4a2da843d6a3d8524a040bd75e33a75c58)
    - Commit [cdaf2623186e464d57cee8838a507ef405a3171a](https://github.com/zephyrproject-rtos/gcc/pull/30/changes/cdaf2623186e464d57cee8838a507ef405a3171a)
      - Reverts [685f45182f4442aff383175c28e8789bbccfceb5](https://github.com/zephyrproject-rtos/gcc/commit/685f45182f4442aff383175c28e8789bbccfceb5)

#### Proposal
Above work, [OS Issue 25569](https://github.com/zephyrproject-rtos/zephyr/issues/25569),
can be used as a boilerplate.

It uses `C11 threads` for GCC's `gthread` implementation.
Note that Zephyr's `C11 threads` implementation uses its `pthreads`,
which in turn uses the kernel API (see above).

Today, it is more straight forward to use `pthreads` directly,
especially since the `GNU Standard C++ Library (libstdc++)`
uses POSIX threads (pthreads) already.

Benefits
- Less code change (`crosstools-ng`, `gcc`)
- Skipping the `C11 threads` wrapper, which uses `pthreads`

#### Other related work
- Closed [cpp: thread: support for std::thread, std::mutex, std::condition_variable, etc #43729](https://github.com/zephyrproject-rtos/zephyr/pull/43729)

#### Current Status
*WIP* currently analyzing, testing custom `crosstool-ng` build ...

TODO
- Following above proposal as outlined

## Misc

### Kconfig
[Kconfig](https://docs.zephyrproject.org/latest/build/kconfig/index.html)
is used to configure the kernel and subsystem at build time.
See [Kconfig option documentation](https://docs.zephyrproject.org/latest/kconfig.html).

To [trace all Kconfig values](https://docs.zephyrproject.org/latest/build/kconfig/tracing.html), invoke

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.sh}
west build -t traceconfig
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

which produces `build/zephyr/kconfig-trace.md`.

To use a [UI Kconfig tool](https://docs.zephyrproject.org/latest/build/kconfig/menuconfig.html) for your project,
invoke on of

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.sh}
# text based
west build -t menuconfig
west build -t menuconfig -b mps2/an385

# graphical based
west build -t guiconfig
west build -t guiconfig -b mps2/an385
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

### Current Kconfig
- [Current Zephyr Kconfig](doc/Zephyr-Kconfig.md)


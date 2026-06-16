/*
 * SPDX-License-Identifier: Apache-2.0 OR MPL-2.0 OR MIT
 *
 * If a copy of the Apache-2.0 license was not distributed with this file,
 * You can obtain one at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * If a copy of the MPL-2.0 license was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * If a copy of the MIT license was not distributed with this file,
 * you can obtain one at https://opensource.org/license/mit/.
 */

#include <zephyr/ztest_assert.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>
#include <sys_malloc.h>

#include <limits>
#include <vector>
#include <stdexcept>
#include <cstdint>

// #define dbg_PRINT(fmt, ...)
#define dbg_PRINT(fmt, ...)                                                                        \
	{                                                                                          \
		fprintf(stderr, "DBG @ alloc_new:%d %s: ", __LINE__, __func__);                	   \
		fprintf(stderr, (fmt)__VA_OPT__(, ) __VA_ARGS__);                                  \
		fputs("\n", stderr);                                                               \
	}

namespace
{
  /** 2 GB 2'147'483'648 (0x80000000), exceeding max 32-bit platform alloc size by 1. */
  constexpr const size_t two_gb = 2UL * 1024UL * 1024UL * 1024UL;

  /** Max 32-bit platform alloc size `2 GB - 1`, i.e. 2'147'483'647 */
  constexpr const size_t oom_size = std::numeric_limits<int>::max();

  constexpr const size_t small_size = 1024UL; // 1kB
  	
  class Small
  {
    public:
      uint8_t data[small_size];

      Small(void) = default;
  };

  class Big
  {
    public:
      uint8_t data[oom_size];

      Big(void) = default;
  };
}

static struct sys_memory_stats malloc_stats;

typedef void (*alloc_op_t)();

static void test_alloc_OK(alloc_op_t alloc_op)
{
	dbg_PRINT("Start");
	try {
		alloc_op();
	} catch (std::exception const &e) {
		dbg_PRINT("exception caught: %s", e.what());
		zassert_unreachable();
	}
	dbg_PRINT("End");
}

static void test_alloc_OOM(bool shall_throw_bad_alloc, alloc_op_t alloc_op)
{
	dbg_PRINT("Start");
	try {
		alloc_op();
	} catch (std::bad_alloc const &oom) {
		if (shall_throw_bad_alloc) {
			dbg_PRINT("expected exception caught: %s", oom.what());
		} else {
			dbg_PRINT("unexpected exception caught: %s", oom.what());
			zassert_unreachable();
		}
	} catch (std::exception const &e) {
		dbg_PRINT("unexpected exception caught: %s", e.what());
		zassert_unreachable();
	}
	dbg_PRINT("End");
}

ZTEST(cpp_basic_alloc_new, test_calloc_chunk_OK)
{
	test_alloc_OK([](){
		uint8_t* mem = (uint8_t*)std::malloc(small_size);
		zassert_not_null(mem);
		std::free(mem);
	});
}

ZTEST(cpp_basic_alloc_new, test_calloc_chunk_OOM)
{
	test_alloc_OOM(false, [](){
		uint8_t* mem = (uint8_t*)std::malloc(malloc_stats.free_bytes+1);
		zassert_is_null(mem);
		std::free(mem);
	});
}

ZTEST(cpp_basic_alloc_new, test_new_array_OK)
{
	test_alloc_OK([](){
		uint8_t* mem = new uint8_t[small_size];
		zassert_not_null(mem);
		delete[] mem;
	});
}

ZTEST(cpp_basic_alloc_new, test_new_array_OOM)
{
	test_alloc_OOM(true, [](){
		uint8_t* mem = new uint8_t[malloc_stats.free_bytes+1];
		zassert_is_null(mem);
		delete[] mem;
		zassert_unreachable();
	});
}

ZTEST(cpp_basic_alloc_new, test_new_vector_OK)
{
	test_alloc_OK([](){
		std::vector<uint8_t> vec(small_size);
		zassert_equal(small_size, vec.size());
	});
}

ZTEST(cpp_basic_alloc_new, test_new_vector_OOM)
{
	test_alloc_OOM(true, [](){
		std::vector<uint8_t> vec(malloc_stats.free_bytes+1);
		zassert_not_equal(malloc_stats.free_bytes+1, vec.size());
		zassert_unreachable();
	});
}

ZTEST(cpp_basic_alloc_new, test_new_obj_OK)
{
	test_alloc_OK([](){
		Small *obj = new Small;
		zassert_not_null(obj);
		delete obj;
	});
}

ZTEST(cpp_basic_alloc_new, test_new_obj_OOM)
{
	zassert_true(oom_size > malloc_stats.free_bytes);
	test_alloc_OOM(true, [](){
		Big *obj = new Big;
		zassert_not_null(obj);
		delete obj;
		zassert_unreachable();
	});
}

static void before(void *arg)
{
	ARG_UNUSED(arg);
	if (!IS_ENABLED(CONFIG_COMMON_LIBC_MALLOC)) {
		ztest_test_skip();
	}
	if (!IS_ENABLED(CONFIG_SYS_HEAP_RUNTIME_STATS)) {
		ztest_test_skip();
	}
	if (!malloc_runtime_stats_get(&malloc_stats)) {
		dbg_PRINT("malloc stats: free %zu MB (%zu bytes)", 
			malloc_stats.free_bytes/size_t(1024UL*1024UL), malloc_stats.free_bytes);
	} else {
		ztest_test_skip();
	}


}

ZTEST_SUITE(cpp_basic_alloc_new, NULL, NULL, before, NULL, NULL);

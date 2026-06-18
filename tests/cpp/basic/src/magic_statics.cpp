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

#include <atomic>
#include <cstddef>
#include <cstdint>

// GNU C++ Library hooks for magic statics
// #include <ext/concurrence.h>

// #define dbg_PRINT(fmt, ...)
#define dbg_PRINT(fmt, ...)                                                                        \
	{                                                                                          \
		fprintf(stderr, "DBG @ magic_statics:%d %s: ", __LINE__, __func__);                \
		fprintf(stderr, (fmt)__VA_OPT__(, ) __VA_ARGS__);                                  \
		fputs("\n", stderr);                                                               \
	}

namespace
{

class TestObj
{
  public:
    uint32_t volatile memberVar;

    static std::atomic<uint8_t> callsToCtor;

    TestObj();
};

std::atomic<uint8_t> TestObj::callsToCtor = 0U;

TestObj::TestObj()
: memberVar(0U)
{
  callsToCtor++;
}


class TestClass
{
  public:
    TestObj* GetStaticTestObjInstance();
};

TestObj* TestClass::GetStaticTestObjInstance()
{
  static TestObj uut;
  return &uut;
}

} // anonymous namespace

ZTEST(cpp_basic_magic_statics, test_magic_statics) // NOLINT
{
  dbg_PRINT("TestObj::callsToCtor %d", TestObj::callsToCtor.load());

  TestClass testClassInstanceA;
  TestClass testClassInstanceB;

  TestObj* p1 = testClassInstanceA.GetStaticTestObjInstance();
  TestObj* p2 = testClassInstanceB.GetStaticTestObjInstance();

  zassert_equal_ptr(p1, p2);
  zassert_equal(TestObj::callsToCtor, 1);

  // number of runs, visible to the debugger
  p1->memberVar++;
  dbg_PRINT("p1->memberVar %d", p1->memberVar);

}

static void before(void *arg)
{
	ARG_UNUSED(arg);
	// No constraints
	// CONFIG_MAX_PTHREAD_COND_COUNT 5 (default)
	dbg_PRINT("CONFIG_MAX_PTHREAD_COND_COUNT %d", CONFIG_MAX_PTHREAD_COND_COUNT);
}

ZTEST_SUITE(cpp_basic_magic_statics, NULL, NULL, before, NULL, NULL);

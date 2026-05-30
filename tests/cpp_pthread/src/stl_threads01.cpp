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

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

// Only works with Zephyr pthread patch https://github.com/zephyrproject-rtos/zephyr/pull/109291
// Only works with the pthread capable SDK toolchain (GCC + libstdc++)
#define TEST_ENABLE_LIBSTDCXX_THREADS 1

#if TEST_ENABLE_LIBSTDCXX_THREADS
#include <thread>
#include <mutex>
#include <condition_variable>
#endif

// #define dbg_PRINT(fmt, ...)
#define dbg_PRINT(fmt, ...)                                                                        \
	{                                                                                          \
		fprintf(stderr, "DBG @ stl_threads01:%d %s: ", __LINE__, __func__);                \
		fprintf(stderr, (fmt)__VA_OPT__(, ) __VA_ARGS__);                                  \
		fputs("\n", stderr);                                                               \
	}

/**
 * Tests a simple producer and consumer threads via C++ Standard Library threads
 * - using mutex lock
 * - using conditional signaling
 * - using TLS
 *
 * Validates basic multithreading and thread-local storage (TLS).
 */

#if TEST_ENABLE_LIBSTDCXX_THREADS

static thread_local int tls_bucket_value = 0;

class Bucket
{
      private:
	std::mutex m;
	std::condition_variable cv;

      public:
	static constexpr const int ItemCount = 50;
	static constexpr const int BucketCapacity = 10;
	volatile int bucket_value = 0;
	volatile int bucket_given = 0;
	volatile int bucket_taken = 0;

      private:
	void TakeItem()
	{
		std::unique_lock<std::mutex> lock(m);
		while (bucket_value == 0) {
			cv.wait(lock);
		}
		// cv.wait(lock, [&](){ return bucket_value>0; });

		--tls_bucket_value;
		bucket_value = bucket_value - 1;
		bucket_taken = bucket_taken + 1;
		if (bucket_taken % 5 == 0 || 1 == bucket_taken) {
			dbg_PRINT("Take: %d/%d, taken %d, given %d, tls %d", bucket_value,
				  ItemCount, bucket_taken, bucket_given, tls_bucket_value);
		}

		zassert_equal(-tls_bucket_value, bucket_taken);
		if (bucket_value == BucketCapacity - 1) {
			lock.unlock();
			cv.notify_all();
		}
	}

	void GiveItem()
	{
		std::unique_lock<std::mutex> lock(m);
		cv.wait(lock, [&] { return bucket_value < BucketCapacity; });

		++tls_bucket_value;
		bucket_value = bucket_value + 1;
		bucket_given = bucket_given + 1;
		if (bucket_given % 5 == 0 || 1 == bucket_given) {
			dbg_PRINT("Give: %d/%d, taken %d, given %d, tls %d", bucket_value,
				  ItemCount, bucket_taken, bucket_given, tls_bucket_value);
		}

		zassert_equal(tls_bucket_value, bucket_given);
		if (bucket_value == 1) {
			lock.unlock();
			cv.notify_all();
		}
	}

      public:
	void ThreadProducer()
	{
		int count = ItemCount;
		dbg_PRINT("START");

		while (count--) {
			GiveItem();
		}
		dbg_PRINT("END");
	}

	void ThreadConsumer()
	{
		int count = ItemCount;
		dbg_PRINT("START");

		while (count--) {
			TakeItem();
		}
		dbg_PRINT("END");
	}
};
#endif

ZTEST(stl_threads01, test_stl_threads01)
{
	dbg_PRINT("MAIN: Thread1");

#if TEST_ENABLE_LIBSTDCXX_THREADS
	dbg_PRINT("MAIN: p0a");

	const size_t thread_count = 2;
	std::thread tasks[thread_count];

	Bucket bucket;
	tasks[0] = std::thread(&Bucket::ThreadProducer, &bucket);
	tasks[1] = std::thread(&Bucket::ThreadConsumer, &bucket);

	dbg_PRINT("MAIN: p1 Bucket: %d/%d, taken %d, given %d", bucket.bucket_value,
		  Bucket::ItemCount, bucket.bucket_taken, bucket.bucket_given);

	for (auto &task : tasks) {
		if (task.joinable()) {
			task.join();
		}
	}
	dbg_PRINT("MAIN: p1 Bucket: %d/%d, taken %d, given %d", bucket.bucket_value,
		  Bucket::ItemCount, bucket.bucket_taken, bucket.bucket_given);
	zassert_equal(Bucket::ItemCount, bucket.bucket_given);
	zassert_equal(Bucket::ItemCount, bucket.bucket_taken);
	zassert_equal(0, bucket.bucket_value);
	zassert_equal(0, tls_bucket_value);

#else
	dbg_PRINT("MAIN: disabled");
#endif
	dbg_PRINT("MAIN: END");
}

static void before(void *arg)
{
	ARG_UNUSED(arg);

#if TEST_ENABLE_LIBSTDCXX_THREADS
	if (!IS_ENABLED(CONFIG_DYNAMIC_THREAD)) {
		/* skip redundant testing if there is no thread pool / heap allocation */
		ztest_test_skip();
	}
#else
	ztest_test_skip();
#endif
}

ZTEST_SUITE(stl_threads01, NULL, NULL, before, NULL, NULL);

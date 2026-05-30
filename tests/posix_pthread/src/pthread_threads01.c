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

#include <pthread.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

// #define dbg_PRINT(fmt, ...)
#define dbg_PRINT(fmt, ...)                                                                        \
	{                                                                                          \
		fprintf(stderr, "DBG @ pthread_threads01:%d %s: ", __LINE__, __func__);            \
		fprintf(stderr, (fmt)__VA_OPT__(, ) __VA_ARGS__);                                  \
		fputs("\n", stderr);                                                               \
	}

/**
 * Tests a simple producer and consumer threads via pthread
 * - using mutex lock
 * - using conditional signaling
 * - using TLS
 *
 * Validates basic multithreading and thread-local storage (TLS).
 */

static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cv = PTHREAD_COND_INITIALIZER;

static const int ItemCount = 50;
static const int BucketCapacity = 10;
static volatile int bucket_value = 0;
static volatile int bucket_given = 0;
static volatile int bucket_taken = 0;

static __thread int tls_bucket_value = 0;

static void TakeItem()
{
	zassert_ok(pthread_mutex_lock(&m));

	while (!bucket_value) {
		zassert_ok(pthread_cond_wait(&cv, &m));
	}
	--tls_bucket_value;
	bucket_value = bucket_value - 1;
	bucket_taken = bucket_taken + 1;
	if (bucket_taken % 5 == 0 || 1 == bucket_taken) {
		dbg_PRINT("Take: %d/%d, taken %d, given %d, tls %d", bucket_value, ItemCount,
			  bucket_taken, bucket_given, tls_bucket_value);
	}

	zassert_equal(-tls_bucket_value, bucket_taken);

	if (bucket_value == BucketCapacity - 1) {
		zassert_ok(pthread_cond_signal(&cv));
	}
	zassert_ok(pthread_mutex_unlock(&m));
}

static void GiveItem()
{
	zassert_ok(pthread_mutex_lock(&m));

	while (bucket_value >= BucketCapacity) {
		zassert_ok(pthread_cond_wait(&cv, &m));
	}
	++tls_bucket_value;
	bucket_value = bucket_value + 1;
	bucket_given = bucket_given + 1;
	if (bucket_given % 5 == 0 || 1 == bucket_given) {
		dbg_PRINT("Give: %d/%d, taken %d, given %d, tls %d", bucket_value, ItemCount,
			  bucket_taken, bucket_given, tls_bucket_value);
	}

	zassert_equal(tls_bucket_value, bucket_given);
	if (bucket_value == 1) {
		zassert_ok(pthread_cond_signal(&cv));
	}
	zassert_ok(pthread_mutex_unlock(&m));
}

static void *ThreadProducer(void *)
{
	int count = ItemCount;
	dbg_PRINT("START");

	while (count--) {
		GiveItem();
	}
	dbg_PRINT("END");
	return NULL;
}

static void *ThreadConsumer(void *)
{
	int count = ItemCount;
	dbg_PRINT("START");

	while (count--) {
		TakeItem();
	}
	dbg_PRINT("END");
	return NULL;
}

ZTEST(pthread_threads01, test_pthread_threads01)
{
	dbg_PRINT("MAIN: p0a");

	pthread_t threadProducer, threadConsumer;
	zassert_ok(pthread_create(&threadConsumer, NULL, &ThreadConsumer, NULL));
	zassert_ok(pthread_create(&threadProducer, NULL, &ThreadProducer, NULL));

	dbg_PRINT("MAIN: p1 Bucket: %d/%d, taken %d, given %d", bucket_value, ItemCount,
		  bucket_taken, bucket_given);
	{
		void *retVal = NULL;
		zassert_ok(pthread_join(threadProducer, &retVal));
		zassert_not_equal(PTHREAD_CANCELED, retVal); // NOLINT(performance-no-int-to-ptr)
		zassert_ok(pthread_join(threadConsumer, &retVal));
		zassert_not_equal(PTHREAD_CANCELED, retVal); // NOLINT(performance-no-int-to-ptr)
	}
	dbg_PRINT("MAIN: p2 Bucket: %d/%d, taken %d, given %d", bucket_value, ItemCount,
		  bucket_taken, bucket_given);
	zassert_equal(ItemCount, bucket_given);
	zassert_equal(ItemCount, bucket_taken);
	zassert_equal(0, bucket_value);
	zassert_equal(0, tls_bucket_value);

	dbg_PRINT("MAIN: END");
}

static void before(void *arg)
{
	ARG_UNUSED(arg);

	if (!IS_ENABLED(CONFIG_DYNAMIC_THREAD)) {
		/* skip redundant testing if there is no thread pool / heap allocation */
		ztest_test_skip();
	}
}

ZTEST_SUITE(pthread_threads01, NULL, NULL, before, NULL, NULL);

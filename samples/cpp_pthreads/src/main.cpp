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

#include <util/miscutils.hpp>

/**
 * Tests a simple producer and consumer threads via pthread
 * - using mutex lock
 * - using conditional signaling
 * - using TLS
 *
 * Validates basic multithreading and thread-local storage (TLS).
 */
namespace
{

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cv = PTHREAD_COND_INITIALIZER;

constexpr const int ItemCount = 50;
constexpr const int BucketCapacity = 10;
volatile int bucket_value = 0;
volatile int bucket_given = 0;
volatile int bucket_taken = 0;

thread_local int tls_bucket_value = 0;

void TakeItem()
{
  if (pthread_mutex_lock(&m) != 0)
  {
    src_ABORT();
  }

  while (!bucket_value)
  {
    if (pthread_cond_wait(&cv, &m) != 0)
    {
      src_ABORT();
    }
  }
  --tls_bucket_value;
  bucket_value = bucket_value - 1;
  bucket_taken = bucket_taken + 1;
  if (bucket_taken % 5 == 0 || 1 == bucket_taken) {
    dbg_PRINT("Take: %d/%d, taken %d, given %d, tls %d", bucket_value, ItemCount, bucket_taken, bucket_given, tls_bucket_value);
  }

  if (-tls_bucket_value != bucket_taken) {
    src_ABORT();
  }
  if (bucket_value == BucketCapacity-1) {
      if (pthread_cond_signal(&cv) != 0)
      {
        src_ABORT();
      }
  }
  if (pthread_mutex_unlock(&m) != 0)
  {
    src_ABORT();
  }
}

void GiveItem()
{
  if (pthread_mutex_lock(&m) != 0)
  {
    src_ABORT();
  }

  while (bucket_value>=BucketCapacity)
  {
    if (pthread_cond_wait(&cv, &m) != 0)
    {
      src_ABORT();
    }
  }
  ++tls_bucket_value;
  bucket_value = bucket_value + 1;
  bucket_given = bucket_given + 1;
  if (bucket_given % 5 == 0 || 1==bucket_given) {
    dbg_PRINT("Give: %d/%d, taken %d, given %d, tls %d", bucket_value, ItemCount, bucket_taken, bucket_given, tls_bucket_value);
  }

  if (tls_bucket_value != bucket_given) {
    src_ABORT();
  }
  if (bucket_value == 1) {
      if (pthread_cond_signal(&cv) != 0)
      {
        src_ABORT();
      }
  }
  if (pthread_mutex_unlock(&m) != 0)
  {
    src_ABORT();
  }
}

static void* ThreadProducer(void*)
{
  int count = ItemCount;
  dbg_PRINT("START");

  while(count--) {
    GiveItem();
  }
  dbg_PRINT("END");
  return nullptr;
}

static void* ThreadConsumer(void*)
{
  int count = ItemCount;
  dbg_PRINT("START");

  while(count--) {
    TakeItem();
  }
  dbg_PRINT("END");
  return nullptr;
}

} // anonymous namespace

int main()
{
  dbg_PRINT("MAIN: p0a");

  pthread_t threadProducer, threadConsumer;
  if (true) {
      int status = pthread_create(&threadConsumer, nullptr, &ThreadConsumer, nullptr);
      if (status != 0)
      {
        fmt_ABORT("pthread_create error %d (%s)", status, strerror(status));
      }
      status = pthread_create(&threadProducer, nullptr, &ThreadProducer, nullptr);
      if (status != 0)
      {
        fmt_ABORT("pthread_create error %d (%s)", status, strerror(status));
      }
  } else {
      int status = pthread_create(&threadProducer, nullptr, &ThreadProducer, nullptr);
      if (status != 0)
      {
        fmt_ABORT("pthread_create error %d (%s)", status, strerror(status));
      }
      status = pthread_create(&threadConsumer, nullptr, &ThreadConsumer, nullptr);
      if (status != 0)
      {
        fmt_ABORT("pthread_create error %d (%s)", status, strerror(status));
      }
  }

  dbg_PRINT("MAIN: p1 Bucket: %d/%d, taken %d, given %d", bucket_value, ItemCount, bucket_taken, bucket_given);
  {
    void* retVal = nullptr;
    if (pthread_join(threadProducer, &retVal) != 0)
    {
      src_ABORT();
    }
    if (PTHREAD_CANCELED == retVal) // NOLINT(performance-no-int-to-ptr)
    {
      src_ABORT();
    }
    if (pthread_join(threadConsumer, &retVal) != 0)
    {
      src_ABORT();
    }
    if (PTHREAD_CANCELED == retVal) // NOLINT(performance-no-int-to-ptr)
    {
      src_ABORT();
    }
  }
  dbg_PRINT("MAIN: p2 Bucket: %d/%d, taken %d, given %d", bucket_value, ItemCount, bucket_taken, bucket_given);
  if (ItemCount != bucket_given || ItemCount != bucket_taken || 0 != bucket_value)
  {
    src_ABORT();
  }
  if (tls_bucket_value != 0) {
    src_ABORT();
  }

  dbg_PRINT("MAIN: END");

  return 0;
}

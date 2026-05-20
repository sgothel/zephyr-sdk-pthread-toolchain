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
#include <iostream>

#include <util/miscutils.hpp>

#include <zephyr/kernel.h>

// Only works with Zephyr pthread patch https://github.com/zephyrproject-rtos/zephyr/pull/109291

extern "C" {
    k_tid_t pthread_to_zephyr_thread(pthread_t pth);
}

#define STACKSIZE 2000
struct k_thread kthread_thread;
struct k_thread *kthread_thread2;
K_THREAD_STACK_DEFINE(kthread__stack, STACKSIZE);

pthread_t pthread_thread;
pthread_t pthread_thread2;

static pthread_key_t key;

static void* pthread_entry(void*)
{
  k_tid_t kself = k_current_get();
  if (!kself) {
    src_ABORT();
  }

  pthread_t pself = pthread_self();
  if (!pself) {
    src_ABORT();
  }
  pthread_thread2 = pself;

  k_tid_t kself2 = pthread_to_zephyr_thread(pself);
  if (kself != kself2) {
    src_ABORT();
  }
  return NULL;
}

void kthread_entry() {
  k_tid_t kself = k_current_get();
  if (!kself) {
    src_ABORT();
  }
  kthread_thread2 = kself;

  pthread_t pself = pthread_self();
  if (!pself) {
    src_ABORT();
  }

  k_tid_t kself2 = pthread_to_zephyr_thread(pself);
  if (kself != kself2) {
    src_ABORT();
  }
}

extern bool pthread_kthread_coop01_test();

bool pthread_kthread_coop01_test()
{
  if (pthread_create(&pthread_thread, NULL, &pthread_entry, NULL) != 0)
  {
    src_ABORT();
  }

  k_thread_create(&kthread_thread, kthread__stack, STACKSIZE,
          (k_thread_entry_t) kthread_entry,
          NULL, NULL, NULL, K_PRIO_COOP(7), 0, K_NO_WAIT);


  if (pthread_join(pthread_thread, NULL) != 0) {
    src_ABORT();
  }
  if (pthread_thread != pthread_thread2) {
    src_ABORT();
  }

  if (k_thread_join(&kthread_thread, K_FOREVER) != 0) {
    src_ABORT();
  }
  if (&kthread_thread != kthread_thread2) {
    src_ABORT();
  }
  return true;
}

int main()
{
    dbg_PRINT("START");

    std::cout << "XXX P0: " << CONFIG_BOARD << std::endl;
    {
      k_tid_t kself = k_current_get();
      if (!kself) {
        src_ABORT();
      }
      pthread_t pself = pthread_self();
      if (!pself) {
        src_ABORT();
      }
      k_tid_t kself2 = pthread_to_zephyr_thread(pself);
      if (kself != kself2) {
        src_ABORT();
      }
      bool r = pthread_kthread_coop01_test();
      dbg_PRINT("XXX P0_2: %d", r);
    }
    dbg_PRINT("END");

    return 0;
}


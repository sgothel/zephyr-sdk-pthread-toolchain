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

#define STACKSIZE 2000
struct k_thread kthread_thread;
K_THREAD_STACK_DEFINE(kthread__stack, STACKSIZE);

pthread_t pthread_thread;

static pthread_key_t key;
static uint8_t const tagP = 0x11U;
static uint8_t const tagK = 0x22U;
static uint8_t const tagM = 0x22U;

static void* pthread_entry(void*)
{
    dbg_PRINT("Thread-P: START");

    pthread_t pself = pthread_self();
    if (!pself) {
        src_ABORT();
    }

    uint8_t *val = (uint8_t *)pthread_getspecific(key);
    if (val) {
        src_ABORT();
    }
    if (0 != pthread_setspecific(key, (void *)&tagP)) {
        src_ABORT();
    }
    val = (uint8_t *)pthread_getspecific(key);
    if (!val) {
        src_ABORT();
    }
    if (val != &tagP) {
        src_ABORT();
    }
    if (val != &tagP) {
        src_ABORT();
    }
    dbg_PRINT("Thread-P: END");
    return NULL;
}

void kthread_entry() {
    dbg_PRINT("Thread-P: START");

    pthread_t pself = pthread_self();
    if (!pself) {
        src_ABORT();
    }

    uint8_t *val = (uint8_t *)pthread_getspecific(key);
    if (val) {
        src_ABORT();
    }
    if (0 != pthread_setspecific(key, (void *)&tagK)) {
        src_ABORT();
    }
    val = (uint8_t *)pthread_getspecific(key);
    if (!val) {
        src_ABORT();
    }
    if (val != &tagK) {
        src_ABORT();
    }
    if (val != &tagK) {
        src_ABORT();
    }
    dbg_PRINT("Thread-K: END");
}

int main()
{
    dbg_PRINT("MAIN: START");

    if (0 != pthread_key_create(&key, nullptr)) {
        src_ABORT();
    }
    uint8_t *val = (uint8_t *)pthread_getspecific(key);
    if (val) {
        src_ABORT();
    }
    if (0 != pthread_setspecific(key, (void *)&tagM)) {
        src_ABORT();
    }
    val = (uint8_t *)pthread_getspecific(key);
    if (!val) {
        src_ABORT();
    }
    if (val != &tagM) {
        src_ABORT();
    }
    if (val != &tagM) {
        src_ABORT();
    }
    dbg_PRINT("MAIN: P1");

    {
        if (pthread_create(&pthread_thread, NULL, &pthread_entry, NULL) != 0) {
            src_ABORT();
        }

        k_thread_create(&kthread_thread, kthread__stack, STACKSIZE,
                        (k_thread_entry_t)kthread_entry, NULL, NULL, NULL,
                        K_PRIO_COOP(7), 0, K_NO_WAIT);

        if (pthread_join(pthread_thread, NULL) != 0) {
            src_ABORT();
        }

        if (k_thread_join(&kthread_thread, K_FOREVER) != 0) {
            src_ABORT();
        }
    }
    dbg_PRINT("END");
    return 0;
}

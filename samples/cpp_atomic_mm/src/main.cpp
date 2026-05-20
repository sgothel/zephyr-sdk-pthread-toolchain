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

#include <atomic>
#include <pthread.h>

#include <cstring>
#include <cstdio>

#include <util/miscutils.hpp>

#define USE_LOGGING 0

#define REQUIRE(a) { if (!(a)) { src_ABORT(); } }

static int loops = 10;

/**
 * Testing SC-DRF non-atomic global read and write within an atomic acquire/release critical block.
 * <p>
 * Modified non-atomic memory within the atomic acquire (load) and release (store) block,
 * must be visible for all threads according to memory model (MM) Sequentially Consistent (SC) being data-race-free (DRF).
 * <br>
 * See Herb Sutter's 2013-12-23 slides p19, first box "It must be impossible for the assertion to fail – wouldn’t be SC.".
 * </p>
 * <p>
 * This test's threads utilize a spin-lock, waiting for their turn.
 * Such busy cycles were chosen to simplify the test and are not recommended
 * as they expose poor performance on a high thread-count and hence long 'working thread pipe'.
 * </p>
 */
class TestMemModelSCDRF00 {
  private:
    enum Defaults : int {
        array_size = 10,
        max_threads = 6
    };
    constexpr int number(const Defaults rhs) noexcept {
        return static_cast<int>(rhs);
    }

    int value1 = 0;
    int array[array_size] = { 0 };
    std::atomic<int> sync_value;

    void reset(int v1, int array_value) {
        int _sync_value = sync_value; // SC-DRF acquire atomic
        (void) _sync_value;
        value1 = v1;
        for(int & i : array) {
            i = array_value;
        }
        sync_value = v1; // SC-DRF release atomic
    }

    void putThreadType01(int _len, int startValue) {
        const int len = std::min(number(array_size), _len);
        {
            int _sync_value = sync_value; // SC-DRF acquire atomic
            (void) _sync_value;
            _sync_value = startValue;
            for(int i=0; i<len; i++) {
                array[i] = _sync_value+i;
            }
            value1 = startValue;
            sync_value = _sync_value; // SC-DRF release atomic
        }
    }
    struct ParamsGetPutThreadType01 {
        TestMemModelSCDRF00 *self;
        int len;
        int startValue;
    };
    static void * staticPutThreadType01(void * vparams) {
       ParamsGetPutThreadType01 *params = (ParamsGetPutThreadType01 *)vparams;
       params->self->putThreadType01(params->len, params->startValue);
       return nullptr;
    }

    void getThreadType01(int _len, int startValue) {
        const int len = std::min(number(array_size), _len);

        int _sync_value;
        while( startValue != ( _sync_value = sync_value ) )
            ; // SC-DRF acquire atomic with spin-lock waiting for startValue
        REQUIRE(sync_value == startValue);
        REQUIRE(startValue == value1);

        for(int i=0; i<len; i++) {
            int v = array[i];
            REQUIRE((startValue+i) == v);
        }
        sync_value = _sync_value; // SC-DRF release atomic (unchanged)
    }
    static void * staticGetThreadType01(void * vparams) {
       ParamsGetPutThreadType01 *params = (ParamsGetPutThreadType01 *)vparams;
       params->self->getThreadType01(params->len, params->startValue);
       return nullptr;
    }

    void putThreadType11(int indexAndValue) {
        const int idx = std::min(number(array_size)-1, indexAndValue);
        {
            // dbg_PRINT("start @ %d", idx);
            // idx is encoded on sync_value (v) as follows
            //   v > 0: get @ idx = v -1
            //   v < 0: put @ idx = abs(v) -1
            int _sync_value;
            // SC-DRF acquire atomic with spin-lock waiting for encoded idx
            do {
                _sync_value = sync_value;
            } while( idx != (_sync_value * -1) - 1 );
            // dbg_PRINT("done @ %d (has %d, exp %d)", idx, _sync_value, (idx+1)*-1);
            _sync_value = idx;
            value1 = idx;
            array[idx] = idx; // last written checked first, SC-DRF should handle...
            sync_value = _sync_value; // SC-DRF release atomic
        }
    }
    struct ParamsGetPutThreadType11 {
        TestMemModelSCDRF00 *self;
        int idx;
    };
    static void * staticPutThreadType11(void * vparams) {
       ParamsGetPutThreadType11 *params = (ParamsGetPutThreadType11 *)vparams;
       params->self->putThreadType11(params->idx);
       delete params;
       return nullptr;
    }
    void getThreadType11(int _idx) {
        const int idx = std::min(number(array_size)-1, _idx);
        // dbg_PRINT("start @ %d", idx);

        // idx is encoded on sync_value (v) as follows
        //   v > 0: get @ idx = v -1
        //   v < 0: put @ idx = abs(v) -1
        int _sync_value;
        // SC-DRF acquire atomic with spin-lock waiting for idx
        do {
            _sync_value = sync_value;
        } while( idx != _sync_value );
        REQUIRE(idx == array[idx]); // check last-written first
        REQUIRE(idx == value1);
        // next write encoded idx
        _sync_value = (idx+1)%array_size;
        _sync_value = ( _sync_value + 1 ) * -1;
        // dbg_PRINT("done for %d, next %d (v %d)", idx, (idx+1)%array_size, _sync_value);
        value1 = _sync_value;
        sync_value = _sync_value; // SC-DRF release atomic
    }
    static void * staticGetThreadType11(void * vparams) {
       ParamsGetPutThreadType11 *params = (ParamsGetPutThreadType11 *)vparams;
       params->self->getThreadType11(params->idx);
       delete params;
       return nullptr;
    }


  public:

    TestMemModelSCDRF00()
    : value1(0), sync_value(0) {}

    void test01_Read1Write1() {
        dbg_PRINT("START.a");
        reset(0, 1010);

        pthread_t threads[2];
        ParamsGetPutThreadType01 p { .self=this, .len = array_size, .startValue = 3};
        int status = pthread_create(&threads[0], nullptr, TestMemModelSCDRF00::staticGetThreadType01, &p);
        if (status != 0) {
          fmt_ABORT("pthread_create error %d (%s)", status, strerror(status));
        }
        status = pthread_create(&threads[1], nullptr, TestMemModelSCDRF00::staticPutThreadType01, &p);
        if (status != 0) {
          fmt_ABORT("pthread_create error %d (%s)", status, strerror(status));
        }
        for (pthread_t &t : threads) {
          void *retVal = nullptr;
          if (pthread_join(t, &retVal) != 0) {
            src_ABORT();
          }
          if (PTHREAD_CANCELED == retVal) // NOLINT(performance-no-int-to-ptr)
          {
            src_ABORT();
          }
        }
    }

    void test02_Read2Write1() {
        dbg_PRINT("START.a");
        reset(0, 1021);
        {
          pthread_t threads[3];
          ParamsGetPutThreadType01 p{.self = this, .len = array_size, .startValue = 4};
          int status = pthread_create(&threads[0], nullptr, TestMemModelSCDRF00::staticGetThreadType01, &p);
          if (status != 0) {
            fmt_ABORT("pthread_create error %d (%s)", status, strerror(status));
          }
          status = pthread_create(&threads[1], nullptr, TestMemModelSCDRF00::staticGetThreadType01, &p);
          if (status != 0) {
            fmt_ABORT("pthread_create error %d (%s)", status, strerror(status));
          }
          status = pthread_create(&threads[2], nullptr, TestMemModelSCDRF00::staticPutThreadType01, &p);
          if (status != 0) {
            fmt_ABORT("pthread_create error %d (%s)", status, strerror(status));
          }
          for (pthread_t &t : threads) {
            void *retVal = nullptr;
            if (pthread_join(t, &retVal) != 0) {
              src_ABORT();
            }
            if (PTHREAD_CANCELED == retVal) // NOLINT(performance-no-int-to-ptr)
            {
              src_ABORT();
            }
          }
        }

        dbg_PRINT("START.b");
        reset(0, 1022);
        {
          pthread_t threads[3];
          ParamsGetPutThreadType01 p{.self = this, .len = array_size, .startValue = 5};
          int status = pthread_create(&threads[0], nullptr, TestMemModelSCDRF00::staticPutThreadType01, &p);
          if (status != 0) {
            fmt_ABORT("pthread_create error %d (%s)", status, strerror(status));
          }
          status = pthread_create(&threads[1], nullptr, TestMemModelSCDRF00::staticGetThreadType01, &p);
          if (status != 0) {
            fmt_ABORT("pthread_create error %d (%s)", status, strerror(status));
          }
          status = pthread_create(&threads[2], nullptr, TestMemModelSCDRF00::staticGetThreadType01, &p);
          if (status != 0) {
            fmt_ABORT("pthread_create error %d (%s)", status, strerror(status));
          }
          for (pthread_t &t : threads) {
            void *retVal = nullptr;
            if (pthread_join(t, &retVal) != 0) {
              src_ABORT();
            }
            if (PTHREAD_CANCELED == retVal) // NOLINT(performance-no-int-to-ptr)
            {
              src_ABORT();
            }
          }
        }
    }

    void test03_Read4Write1() {
        dbg_PRINT("START");
        reset(0, 1030);

        {
          pthread_t threads[5];
          ParamsGetPutThreadType01 p{.self = this, .len = array_size, .startValue = 6};
          int status = pthread_create(&threads[0], nullptr, TestMemModelSCDRF00::staticGetThreadType01, &p);
          if (status != 0) {
            fmt_ABORT("pthread_create error %d (%s)", status, strerror(status));
          }
          status = pthread_create(&threads[1], nullptr, TestMemModelSCDRF00::staticGetThreadType01, &p);
          if (status != 0) {
            fmt_ABORT("pthread_create error %d (%s)", status, strerror(status));
          }
          status = pthread_create(&threads[2], nullptr, TestMemModelSCDRF00::staticPutThreadType01, &p);
          if (status != 0) {
            fmt_ABORT("pthread_create error %d (%s)", status, strerror(status));
          }
          status = pthread_create(&threads[3], nullptr, TestMemModelSCDRF00::staticGetThreadType01, &p);
          if (status != 0) {
            fmt_ABORT("pthread_create error %d (%s)", status, strerror(status));
          }
          status = pthread_create(&threads[4], nullptr, TestMemModelSCDRF00::staticGetThreadType01, &p);
          if (status != 0) {
            fmt_ABORT("pthread_create error %d (%s)", status, strerror(status));
          }
          for (pthread_t &t : threads) {
            void *retVal = nullptr;
            if (pthread_join(t, &retVal) != 0) {
              src_ABORT();
            }
            if (PTHREAD_CANCELED == retVal) // NOLINT(performance-no-int-to-ptr)
            {
              src_ABORT();
            }
          }
        }
    }

    void test11_Read10Write10() {
        dbg_PRINT("START");
        reset(-1, 1110); // start put idx 0

        pthread_t reader[max_threads/2], writer[max_threads/2];
        for(int i=0; i<number(max_threads)/2; i++) {
          ParamsGetPutThreadType11 *p = new ParamsGetPutThreadType11{.self = this, .idx = i};
          int status = pthread_create(&reader[i], nullptr, TestMemModelSCDRF00::staticGetThreadType11, p);
          if (status != 0) {
            fmt_ABORT("pthread_create %d: error %d (%s)", i, status, strerror(status));
          }
        }
        for(int i=0; i<number(max_threads)/2; i++) {
          ParamsGetPutThreadType11 *p = new ParamsGetPutThreadType11{.self = this, .idx = i};
          int status = pthread_create(&writer[i], nullptr, TestMemModelSCDRF00::staticPutThreadType11, p);
          if (status != 0) {
            fmt_ABORT("pthread_create %d: error %d (%s)", i, status, strerror(status));
          }
        }
        for(int i=0; i<number(max_threads)/2; i++) {
            // dbg_PRINT("p1 %d", i);
            void *retVal = nullptr;
            if (pthread_join(writer[i], &retVal) != 0) {
              src_ABORT();
            }
            if (PTHREAD_CANCELED == retVal) // NOLINT(performance-no-int-to-ptr)
            {
              src_ABORT();
            }
        }
        for(int i=0; i<number(max_threads)/2; i++) {
            // dbg_PRINT("p2 %d", i);
            void *retVal = nullptr;
            if (pthread_join(reader[i], &retVal) != 0) {
              src_ABORT();
            }
            if (PTHREAD_CANCELED == retVal) // NOLINT(performance-no-int-to-ptr)
            {
              src_ABORT();
            }
        }
    }

    void test12_Read10Write10() {
        dbg_PRINT("START");
        reset(-1, 1120); // start put idx 0

        pthread_t reader[max_threads/2], writer[max_threads/2];
        for(int i=0; i<number(max_threads)/2; i++) {
          ParamsGetPutThreadType11 *p = new ParamsGetPutThreadType11{.self = this, .idx = i};
          int status = pthread_create(&writer[i], nullptr, TestMemModelSCDRF00::staticPutThreadType11, p);
          if (status != 0) {
            fmt_ABORT("pthread_create %d: error %d (%s)", i, status, strerror(status));
          }
        }
        for(int i=0; i<number(max_threads)/2; i++) {
          ParamsGetPutThreadType11 *p = new ParamsGetPutThreadType11{.self = this, .idx = i};
          int status = pthread_create(&reader[i], nullptr, TestMemModelSCDRF00::staticGetThreadType11, p);
          if (status != 0) {
            fmt_ABORT("pthread_create %d: error %d (%s)", i, status, strerror(status));
          }
        }
        for(int i=0; i<number(max_threads)/2; i++) {
            // dbg_PRINT("p1 %d", i);
            void *retVal = nullptr;
            if (pthread_join(writer[i], &retVal) != 0) {
              src_ABORT();
            }
            if (PTHREAD_CANCELED == retVal) // NOLINT(performance-no-int-to-ptr)
            {
              src_ABORT();
            }
        }
        for(int i=0; i<number(max_threads)/2; i++) {
            // dbg_PRINT("p2 %d", i);
            void *retVal = nullptr;
            if (pthread_join(reader[i], &retVal) != 0) {
              src_ABORT();
            }
            if (PTHREAD_CANCELED == retVal) // NOLINT(performance-no-int-to-ptr)
            {
              src_ABORT();
            }
        }
    }

    void test_list() {
        for(int i=loops; i>0; i--) { test01_Read1Write1(); }
        for(int i=loops; i>0; i--) { test02_Read2Write1(); }
        for(int i=loops; i>0; i--) { test03_Read4Write1(); }
        for(int i=loops; i>0; i--) { test11_Read10Write10(); }
        for(int i=loops; i>0; i--) { test12_Read10Write10(); }
    }
};

int main()
{
  dbg_PRINT("MAIN: START");
  TestMemModelSCDRF00 ta;
  ta.test_list();
  dbg_PRINT("MAIN: END");
  return 0;
}

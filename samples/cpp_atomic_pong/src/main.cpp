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

#include <util/miscutils.hpp>

static thread_local int tls_counter = 0;

#define USE_LOGGING 0

#define REQUIRE(a) { if (!(a)) { src_ABORT(); } }

/**
 * test_atomics_00: Tests a simple ping/pong based on atomics w/ spin-locks and atomic-wait from 2 threads
 * - plain atomics w/ spin-lock (busy loop) or atomic-wait
 * - using TLS
 * - no mutex lock
 * - no conditional signaling
 *
 * Validates basic atomic thread-safety and thread-local storage (TLS).
 */
template<bool USE_CPP20_ATOMIC_WAIT>
class TestAtomics00 {
  private:
    typedef TestAtomics00 Self;

    template <typename T>
    static void wait(const T &atom, typename T::value_type __old) noexcept {
        if constexpr (USE_CPP20_ATOMIC_WAIT) {
            atom.wait(__old); // atomic-wait, blocks until notified
        } else {
            // spin-lock waiting
            typename T::value_type __val = atom;
            while (__val == __old) {
                __val = atom;
            }
        }
    }

  #if USE_LOGGING == 0

    // no logging

    template <typename T>
    static typename T::value_type wait_for(const T &atom, typename T::value_type __new) noexcept {
        if constexpr (USE_CPP20_ATOMIC_WAIT) {
            return atom.wait_for(__new);
        } else {
            // spin-lock waiting
            const typename T::value_type __start = atom;
            typename T::value_type __old = __start;
            while (__new != __old) {
                __old = atom;
            }
            return __start;
        }
    }

    template <typename T>
    static void notify_one(T &atom) noexcept {
        if constexpr (USE_CPP20_ATOMIC_WAIT) {
            atom.notify_one();
        } else {
            // else spin-lock busy loop used
            (void) atom;
        }
    }

  #else

    // with logging

    template <typename T>
    static typename T::value_type wait_for(const T &atom, typename T::value_type __new) noexcept {
        if constexpr (USE_CPP20_ATOMIC_WAIT) {
            const typename T::value_type __start = atom;
            typename T::value_type __old = __start;
            while (__new != __old) {
                dbg_PRINT("w0: old %d != new %d", __old, __new);
                atom.wait(__old);
                __old = atom;
            }
            dbg_PRINT("wX: old %d == new %d", __old, __new);
            return __start;
        } else {
            // spin-lock waiting
            const typename T::value_type __start = atom;
            typename T::value_type __old = __start;
            while (__new != __old) {
                dbg_PRINT("s0: old %d != new %d", __old, __new);
                __old = atom;
            }
            dbg_PRINT("sX: old %d == new %d", __old, __new);
            return __start;
        }
    }

    template <typename T>
    static void notify_one(T &atom) noexcept {
        if constexpr (USE_CPP20_ATOMIC_WAIT) {
            dbg_PRINT("W: %d", atom.load());
            atom.notify_one();
        } else {
            // else spin-lock busy loop used
            dbg_PRINT("S: %d", atom.load());
            (void) atom;
        }
    }

  #endif

    static constexpr const int PingCount = 50;
    static constexpr const int IDLE = 0;
    static constexpr const int PING = 1;
    static constexpr const int PONG = 2;

    std::atomic<int> pingpong_counter = 0;
    std::atomic<int> pingpong_status = IDLE;
    std::atomic<int> threads_ready = 0; // ensure both threads have started, allowing atomic-wait being notified

    struct PingPong {
        int status = IDLE;
        int total_counter = 0;
        int ping_counter = 0;
        int pong_counter = 0;
    };
    // std::atomic<PingPong> pingpong;
    PingPong pingpong;

    void reset(int status) {
        threads_ready = 0;
        PingPong pp;
        pp.status = status;
        pp.total_counter = 0;
        pp.ping_counter = 0;
        pp.pong_counter = 0;
        pingpong = pp;
        pingpong_counter = 0;
        pingpong_status = status;
        tls_counter = 0;
    }

    void ping() {
      int count = PingCount;
      ++threads_ready;
      dbg_PRINT("START: %d", threads_ready.load());
      notify_one(threads_ready);

      while(count--) {
          wait_for(pingpong_status, PING);

          REQUIRE (pingpong_status == PING);
          PingPong pp = pingpong;
          REQUIRE (pp.status == PING);
          REQUIRE (pingpong_counter == pp.total_counter);
          REQUIRE (pp.total_counter == pp.ping_counter + pp.pong_counter);
          REQUIRE (tls_counter == pp.ping_counter);

          pingpong_counter = pingpong_counter + 1;
          ++tls_counter;
          ++pp.ping_counter;
          ++pp.total_counter;
          pp.status = PONG;

          #if USE_LOGGING
            dbg_PRINT("ping: ping_counter %d, left %d", pp.ping_counter, count);
          #endif

          pingpong = pp;
          pingpong_status = PONG;
          notify_one(pingpong_status);
      }
      dbg_PRINT("END");
    }
    static void * staticPing(void * This) {
       ((Self *)This)->ping();
       return nullptr;
    }

    void pong() {
      int count = PingCount;
      ++threads_ready;
      dbg_PRINT("START: %d", threads_ready.load());
      notify_one(threads_ready);

      while(count--) {
          wait_for(pingpong_status, PONG);

          REQUIRE (pingpong_status == PONG);
          PingPong pp = pingpong;
          REQUIRE (pp.status == PONG);
          REQUIRE (pingpong_counter == pp.total_counter);
          REQUIRE (pp.total_counter == pp.ping_counter + pp.pong_counter);
          REQUIRE (tls_counter == pp.pong_counter);

          pingpong_counter = pingpong_counter + 1;
          ++tls_counter;
          ++pp.pong_counter;
          ++pp.total_counter;
          pp.status = count ? PING : IDLE;

          #if USE_LOGGING
            dbg_PRINT("pong: pong_counter %d, left %d", pp.pong_counter, count);
          #endif

          pingpong = pp;
          pingpong_status = count ? PING : IDLE;
          notify_one(pingpong_status);
      }
      dbg_PRINT("END");
    }
    static void * staticPong(void * This) {
       ((Self *)This)->pong();
       return nullptr;
    }

  public:
    void test01(bool startPongFirst) {
      dbg_PRINT("MAIN: p0 startPongFirst %d, is_lock_free (obj %d, type %d), USE_CPP20_ATOMIC_WAIT %d",
        startPongFirst, pingpong_status.is_lock_free(), std::atomic<int>::is_always_lock_free, USE_CPP20_ATOMIC_WAIT);

      reset(PING);

      pthread_t threads[2];
      if (startPongFirst) {
          int status = pthread_create(&threads[0], nullptr, TestAtomics00::staticPong, this);
          if (status != 0)
          {
            fmt_ABORT("pthread_create error %d (%s)", status, strerror(status));
          }
          status = pthread_create(&threads[1], nullptr, TestAtomics00::staticPing, this);
          if (status != 0)
          {
            fmt_ABORT("pthread_create error %d (%s)", status, strerror(status));
          }
      } else {
          int status = pthread_create(&threads[0], nullptr, TestAtomics00::staticPing, this);
          if (status != 0)
          {
            fmt_ABORT("pthread_create error %d (%s)", status, strerror(status));
          }
          status = pthread_create(&threads[1], nullptr, TestAtomics00::staticPong, this);
          if (status != 0)
          {
            fmt_ABORT("pthread_create error %d (%s)", status, strerror(status));
          }
      }

      PingPong pp = pingpong;
      dbg_PRINT("MAIN: p1 threads %d, total: %d/%d, ping %d, pong %d",
        threads_ready.load(), pp.total_counter, PingCount, pp.ping_counter, pp.pong_counter);

      wait_for(threads_ready, 2); // ensure both threads have started, allowing atomic-wait being notified

      dbg_PRINT("MAIN: p2 threads %d, total: %d/%d, ping %d, pong %d",
        threads_ready.load(), pp.total_counter, PingCount, pp.ping_counter, pp.pong_counter);

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
      pp = pingpong;
      dbg_PRINT("MAIN: p3 total: %d/%d, ping %d, pong %d", pp.total_counter, PingCount, pp.ping_counter, pp.pong_counter);
      REQUIRE (PingCount == pp.ping_counter);
      REQUIRE (PingCount == pp.pong_counter);
      REQUIRE (2*PingCount == pp.total_counter);
      REQUIRE (2*PingCount == pingpong_counter);
      REQUIRE (pp.status == IDLE);
      REQUIRE (pingpong_status == IDLE);

      REQUIRE (tls_counter == 0);


      dbg_PRINT("MAIN: XXX");
    }
};

static int loops = 4;

static void test_atomics_spin_lock() {
    bool startPongFirst = true;
    for(int i=loops; i>0; i--) {
        TestAtomics00<false> ta;
        ta.test01(startPongFirst);
        startPongFirst = !startPongFirst;
    }
}

#if 0 && __cplusplus > 201703L // C++20
    static void test_atomics_atomic_wait() {
        bool startPongFirst = true;
        for(int i=loops; i>0; i--) {
            TestAtomics00<true> ta;
            ta.test01(startPongFirst);
            startPongFirst = !startPongFirst;
        }
    }
#endif

int main()
{
  dbg_PRINT("MAIN: START");
  test_atomics_spin_lock();
  #if 0 && __cplusplus > 201703L // C++20
    test_atomics_atomic_wait();
  #else
    dbg_PRINT("MAIN: AtomicWait n/a - SKIPPED");
  #endif
  dbg_PRINT("MAIN: END");
  return 0;
}

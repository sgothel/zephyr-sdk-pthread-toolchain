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
#include <stdexcept>
#include <exception>
#include <cstdint>
#include <cstring>
#include <cstdio>

#include <util/miscutils.hpp>

namespace
{

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cv = PTHREAD_COND_INITIALIZER;

enum class State
{
  WaitingForThread2_ProceedTo_A,
  Thread2_Reached_A,

  WaitingForThread2_ProceedTo_B,
  Thread2_Reached_B,

  WaitingForThread2_Terminate
};

// "m" required. "cv" for signalling change.
State state;

uint8_t const tagSuccess = 42U;

// If we make things depending on this being 13, then we can prevent some compiler optimizations
uint8_t volatile thirteen = 13U;

void WaitState(State const waitingFor)
{
  if (pthread_mutex_lock(&m) != 0)
  {
    src_ABORT();
  }
  dbg_PRINT("WaitState.0: %d, wait %d", (int)state, (int)waitingFor);

  while (state != waitingFor)
  {
    if (pthread_cond_wait(&cv, &m) != 0)
    {
      src_ABORT();
    }
  }
  dbg_PRINT("WaitState.X: %d", (int)state);

  if (pthread_mutex_unlock(&m) != 0)
  {
    src_ABORT();
  }
}

void SetStateSignalAndWait(State const next, State const waitingFor)
{
  if (pthread_mutex_lock(&m) != 0)
  {
    src_ABORT();
  }

  dbg_PRINT("SetStateSignalAndWait.0: %d -> %d, wait %d", (int)state, (int)next, (int)waitingFor);
  state = next;
  if (pthread_cond_signal(&cv) != 0)
  {
    src_ABORT();
  }

  while (state != waitingFor)
  {
    if (pthread_cond_wait(&cv, &m) != 0)
    {
      src_ABORT();
    }
  }
  dbg_PRINT("SetStateSignalAndWait.X: %d", (int)state);

  if (pthread_mutex_unlock(&m) != 0)
  {
    src_ABORT();
  }
}

constexpr const unsigned int CookieAValue = 0xAFFEAFFEU;
constexpr const unsigned int CookieBValue = 0xDEADBEEFU;

constexpr const int ExceptionA_ID = 1;
constexpr const int ExceptionB_ID = 2;
constexpr const int ExceptionC_ID = 3;

constexpr const char *ExceptionA_Name = "Exception A";
constexpr const char *ExceptionB_Name = "Exception B";
constexpr const char *ExceptionC_Name = "Exception C";
constexpr const size_t ExceptionNameLen = 12;

/// Local copy of exception message, increased footprint and triggers error earlier
#define LOCAL_EXCEPTION_MESSAGE 0


/**
 * Size: 24 bytes, i.e. 6*sizeof(void*)
 *
 * GCC `--with-libstdcxx-eh-pool-obj-count=NUM`,
 * see [GCC libstdc++ exceptions manual](https://gcc.gnu.org/onlinedocs/libstdc++/manual/using_exceptions.html):
 * <pre>
 * size of the pool will be sufficient for NUM exceptions of 6 * sizeof(void*) bytes,
 * plus another NUM exceptions captured in std::exception_ptr and rethrown using std::rethrow_exception.
 * </pre>
 */
class RuntimeException : public std::logic_error {
  private:
    unsigned int cookie1a = CookieAValue;
    short m_id;
    short m_line;
    pthread_t m_thread;
    // const char *m_file;
#if LOCAL_EXCEPTION_MESSAGE
    char m_msg[32];
#endif
    unsigned int cookie2a = CookieAValue;

  public:
    RuntimeException(short id, const char *msg, const char*, int line) noexcept
    : std::logic_error(msg),
      m_id(id), m_line((short)line),
      m_thread(pthread_self())
      // , m_file(file)
    {
#if LOCAL_EXCEPTION_MESSAGE
        strncpy(m_msg, msg, sizeof(m_msg)-1);
        m_msg[sizeof(m_msg)-1] = 0; // EOS
#endif
    }

    ~RuntimeException() noexcept override = default; // virtual
    RuntimeException(const RuntimeException& o) noexcept = default;
    RuntimeException(RuntimeException&& o) noexcept = default;
    RuntimeException& operator=(const RuntimeException& o) noexcept = default;
    RuntimeException& operator=(RuntimeException&& o) noexcept = default;

    const char* message() const noexcept {
#if LOCAL_EXCEPTION_MESSAGE
        return m_msg;
#else
        return std::logic_error::what();
#endif
    }
    const char* what() const noexcept override { return message(); }

    constexpr int id() const noexcept { return m_id; }
    constexpr size_t thread() const noexcept { return m_thread; }
    constexpr const char* file() const noexcept { return "file"; } // m_file; }
    constexpr int line() const noexcept { return m_line; }

    /** Returns whole description to new string */
    std::string toString() const noexcept {
        return util::unsafe::format_string_n(256, "REx[id %hd, a %p, ck %d, src[t %zx @ `%s`:%hd]]: `%s`",
            m_id, this, checkCookies(), thread(), file(), m_line, message());
    }
    bool checkCookies() const noexcept {
      return CookieAValue == cookie1a && CookieAValue == cookie2a;
    }
    void cookiesAssert(int line) const {
      bool c1 = CookieAValue == cookie1a;
      bool c2 = CookieAValue == cookie2a;
      if (!c1 || !c2) {
         dbg_PRINT("Potentially garbage: REx[id %hd, a %p, src[t %zx @ line %hd]]", m_id, this, thread(), m_line);
         fmt_ABORT("cookie 1 (ok %d) 0x%x, 2 (ok %d) 0x%x, checked @ line %d", c1, cookie1a, c2, cookie2a, line);
      }
    }
    bool match(int id) const noexcept { return m_id == id; }
    bool match(const char *msg) const noexcept {
        size_t message_len = strnlen(message(), ExceptionNameLen);
        size_t msg_len = strnlen(msg, ExceptionNameLen);
        if( message_len != msg_len ) {
            return false;
        }
        return 0 == strncmp(message(), msg, msg_len);
    }
    bool match(int id, const char *msg) const noexcept { return match(id) && match(msg); }

    std::ostream& operator<<(std::ostream& out) noexcept {
        return out << toString();
    }
};

void Test_cpp_ExceptionBasics() {
    {
        dbg_PRINT("Size: RuntimeException %zu (/ %zu sizeof(void*) = %zu)]",
            sizeof(RuntimeException), sizeof(void*), sizeof(RuntimeException)/sizeof(void*));
        dbg_PRINT("Size: std::logic_error %zu (/ %zu sizeof(void*) = %zu)]",
            sizeof(std::logic_error), sizeof(void*), sizeof(std::logic_error)/sizeof(void*));
    }
    std::logic_error e0 = std::logic_error(ExceptionB_Name);
    std::invalid_argument e1 = std::invalid_argument(ExceptionC_Name);
    RuntimeException e2(ExceptionA_ID, ExceptionA_Name, E_FILE_LINE);
    RuntimeException e3(ExceptionB_ID, ExceptionB_Name, E_FILE_LINE);
    {
        std::exception const & e = e0;
        dbg_PRINT("p0a: what `%s`, id `%s`, hash 0x%zx", e.what(), typeid(e).name(), typeid(e).hash_code());
        if (strncmp(e.what(), ExceptionB_Name, ExceptionNameLen) != 0)
        {
          fmt_ABORT("exception msg not `Exception B` but `%s`", e.what());
        }
        if (typeid(e) != typeid(std::logic_error))
        {
          fmt_ABORT("exception not std::logic_error but `%s`", typeid(e).name());
        }
    }
    {
      std::exception const &e = e1;
      dbg_PRINT("p0b: what `%s`, id `%s`, hash 0x%zx", e.what(), typeid(e).name(), typeid(e).hash_code());
      if (strncmp(e.what(), ExceptionC_Name, ExceptionNameLen) != 0)
      {
        fmt_ABORT("exception msg not `Exception C` but `%s`", e.what());
      }
      if (typeid(e) != typeid(std::invalid_argument))
      {
        fmt_ABORT("exception not std::invalid_argument but `%s`", typeid(e).name());
      }
    }
    {
      RuntimeException const &e = e2;
      dbg_PRINT("p0c: %s, id `%s`, hash 0x%zx", e.toString().c_str(), typeid(e).name(), typeid(e).hash_code());
      if ( !e.match(ExceptionA_ID, ExceptionA_Name) )
      {
        fmt_ABORT("exception mismatch: %s", e.toString().c_str());
      }
      if (typeid(e) != typeid(RuntimeException))
      {
        fmt_ABORT("exception not RuntimeException but `%s`", typeid(e).name());
      }
    }
    {
      RuntimeException const &e = e3;
      dbg_PRINT("p0d: %s, id `%s`, hash 0x%zx", e.toString().c_str(), typeid(e).name(), typeid(e).hash_code());
      if ( !e.match(ExceptionB_ID, ExceptionB_Name) )
      {
        fmt_ABORT("exception mismatch: %s", e.toString().c_str());
      }
      if (typeid(e) != typeid(RuntimeException))
      {
        fmt_ABORT("exception not RuntimeException but `%s`", typeid(e).name());
      }
    }
}

void Thread2Inner()
{
  try
  {
    dbg_PRINT("T2i: p1");
    util::lang::callNotOptimize([]() {
      // Prevent any prediction and optimization by the compiler. "thirteen" is of course 13, but it is volatile.
      if (thirteen == 13U) {
        dbg_PRINT("T2i: p2c"); // NOLINT
        const pthread_t self = pthread_self();
        dbg_PRINT("T2i: p2d 0x%zx", self); // NOLINT
        if (!self) {
          src_ABORT(); // NOLINT
        }
        throw RuntimeException(ExceptionA_ID, ExceptionA_Name, E_FILE_LINE);
      }
    });
  } catch (RuntimeException const &e) {
    // ###########
    // # Point A #
    // ###########
    [[maybe_unused]] std::exception_ptr eptr;
    e.cookiesAssert(__LINE__);
    dbg_PRINT("T2i: p3: %s, id `%s`, hash 0x%zx", e.toString().c_str(), typeid(e).name(), typeid(e).hash_code());
    if ( !e.match(ExceptionA_ID, ExceptionA_Name) )
    {
      fmt_ABORT("exception mismatch: %s", e.toString().c_str());
    }
    if (typeid(e) != typeid(RuntimeException))
    {
      fmt_ABORT("exception not RuntimeException but `%s`", typeid(e).name());
    }

    SetStateSignalAndWait(State::Thread2_Reached_A, State::WaitingForThread2_ProceedTo_B);

    e.cookiesAssert(__LINE__);
    dbg_PRINT("T2i: p4: %s, id `%s`, hash 0x%zx", e.toString().c_str(), typeid(e).name(), typeid(e).hash_code());
    if ( !e.match(ExceptionA_ID, ExceptionA_Name) )
    {
      fmt_ABORT("exception mismatch: %s", e.toString().c_str());
    }
    if (typeid(e) != typeid(RuntimeException))
    {
      fmt_ABORT("exception not RuntimeException but `%s`", typeid(e).name());
    }

    dbg_PRINT("T2i: p5");
    throw; // ERROR-1
  } catch (...) {
    std::exception_ptr eptr = std::current_exception();
    util::handle_exception(eptr, E_FILE_LINE);
    src_ABORT();
  }
  src_ABORT();
}

static void* Thread2Entry(void*)
{
  dbg_PRINT("START: Thread2");

  try
  {
    Thread2Inner();
  }
  catch (RuntimeException const & e)
  {
    // ###########
    // # Point B #
    // ###########
    e.cookiesAssert(__LINE__);
    dbg_PRINT("T2e: p3: %s, id `%s`, hash 0x%zx", e.toString().c_str(), typeid(e).name(), typeid(e).hash_code());

    if ( !e.match(ExceptionA_ID, ExceptionA_Name) )
    {
      fmt_ABORT("exception mismatch: %s", e.toString().c_str());
    }
    if (typeid(e) != typeid(RuntimeException))
    {
      fmt_ABORT("exception not RuntimeException but `%s`", typeid(e).name());
    }

    SetStateSignalAndWait(State::Thread2_Reached_B, State::WaitingForThread2_Terminate);

    e.cookiesAssert(__LINE__);
    dbg_PRINT("T2e: p4: %s, id `%s`, hash 0x%zx", e.toString().c_str(), typeid(e).name(), typeid(e).hash_code());

    if ( !e.match(ExceptionA_ID, ExceptionA_Name) )
    {
      fmt_ABORT("exception mismatch: %s", e.toString().c_str());
    }
    if (typeid(e) != typeid(RuntimeException))
    {
      fmt_ABORT("exception not RuntimeException but `%s`", typeid(e).name());
    }
    dbg_PRINT("T2e: XXX OK");
    return (void*)&tagSuccess;
  } catch (...) {
    std::exception_ptr eptr = std::current_exception();
    util::handle_exception(eptr, E_FILE_LINE);
    src_ABORT();
  }
  src_ABORT();
  return nullptr;
}

} // anonymous namespace


int main()
{
  dbg_PRINT("MAIN: START");
  Test_cpp_ExceptionBasics();

  dbg_PRINT("MAIN: p0b");
  SetStateSignalAndWait(State::WaitingForThread2_ProceedTo_A, State::WaitingForThread2_ProceedTo_A);

  dbg_PRINT("MAIN: p0b");
  pthread_t thread;
  {
      int status = pthread_create(&thread, nullptr, &Thread2Entry, nullptr);
      if (status != 0)
      {
        fmt_ABORT("pthread_create error %d (%s)", status, strerror(status));
      }
  }

  dbg_PRINT("MAIN: p1");
  WaitState(State::Thread2_Reached_A);

  volatile unsigned int cookie1a = CookieAValue;
  volatile unsigned int cookie1b = CookieBValue;
  try
  {
    try
    {
      dbg_PRINT("MAIN: p2a");
      {
        const pthread_t self = pthread_self();
        dbg_PRINT("MAIN: p2b 0x%zx", self);
        if (!self) {
          src_ABORT();
        }
      }
      util::lang::callNotOptimize([]() {
        // Prevent any prediction and optimization by the compiler. "thirteen"
        // is of course 13, but it is volatile.
        if (thirteen == 13U) {
          dbg_PRINT("MAIN: p2c"); // NOLINT
          const pthread_t self = pthread_self();
          dbg_PRINT("MAIN: p2d 0x%zx", self); // NOLINT
          if (!self) {
            src_ABORT(); // NOLINT
          }
          throw RuntimeException(ExceptionB_ID, ExceptionB_Name, E_FILE_LINE);
        }
      });
    }
    catch (RuntimeException const & e)
    {
      volatile unsigned int cookie2a = CookieAValue;
      volatile unsigned int cookie2b = CookieBValue;
      e.cookiesAssert(__LINE__);
      dbg_PRINT("MAIN: p3: %s, id `%s`, hash 0x%zx", e.toString().c_str(), typeid(e).name(), typeid(e).hash_code());
      if ( !e.match(ExceptionB_ID, ExceptionB_Name) )
      {
        fmt_ABORT("exception mismatch: %s", e.toString().c_str());
      }
      dbg_PRINT("MAIN: p3c");
      if (typeid(e) != typeid(RuntimeException))
      {
        fmt_ABORT("exception not RuntimeException but `%s`", typeid(e).name());
      }
      dbg_PRINT("MAIN: p3d");

      SetStateSignalAndWait(State::WaitingForThread2_ProceedTo_B, State::Thread2_Reached_B); // Corrupts 'e'
      dbg_PRINT("MAIN: p3e");

      if (CookieAValue != cookie1a || CookieBValue != cookie1b ) {
         fmt_ABORT("cookie 0x%xu, 0x%xu", cookie1a, cookie1b);
      }
      if (CookieAValue != cookie2a || CookieBValue != cookie2b ) {
         fmt_ABORT("cookie 0x%xu, 0x%xu", cookie2a, cookie2b);
      }

      e.cookiesAssert(__LINE__);
      dbg_PRINT("MAIN: p4: %s, id `%s`, hash 0x%zx", e.toString().c_str(), typeid(e).name(), typeid(e).hash_code());
      if ( !e.match(ExceptionB_ID, ExceptionB_Name) )
      {
        fmt_ABORT("exception mismatch: %s", e.toString().c_str());
      }
      dbg_PRINT("MAIN: p4b");
      if (typeid(e) != typeid(RuntimeException))
      {
        fmt_ABORT("exception not RuntimeException but `%s`", typeid(e).name());
      }

      dbg_PRINT("MAIN: p5");
      throw RuntimeException(ExceptionC_ID, ExceptionC_Name, E_FILE_LINE);
    } catch (...) {
      std::exception_ptr eptr = std::current_exception();
      util::handle_exception(eptr, E_FILE_LINE);
      src_ABORT();
    }
    src_ABORT();
  }
  catch (RuntimeException const & e)
  {
    volatile unsigned int cookie2a = CookieAValue;
    volatile unsigned int cookie2b = CookieBValue;
    e.cookiesAssert(__LINE__);
    dbg_PRINT("MAIN: p11: %s, id `%s`, hash 0x%zx", e.toString().c_str(), typeid(e).name(), typeid(e).hash_code());
    if ( !e.match(ExceptionC_ID, ExceptionC_Name) )
    {
      fmt_ABORT("exception mismatch: %s", e.toString().c_str());
    }
    dbg_PRINT("MAIN: p11c");
    if (typeid(e) != typeid(RuntimeException))
    {
      fmt_ABORT("exception not RuntimeException but `%s`", typeid(e).name());
    }

    SetStateSignalAndWait(State::WaitingForThread2_Terminate, State::WaitingForThread2_Terminate);

    if (CookieAValue != cookie1a || CookieBValue != cookie1b) {
      fmt_ABORT("cookie 0x%xu, 0x%xu", cookie1a, cookie1b);
    }
    if (CookieAValue != cookie2a || CookieBValue != cookie2b) {
      fmt_ABORT("cookie 0x%xu, 0x%xu", cookie2a, cookie2b);
    }

    e.cookiesAssert(__LINE__);
    dbg_PRINT("MAIN: p12a: %s, id `%s`, hash 0x%zx", e.toString().c_str(), typeid(e).name(), typeid(e).hash_code());
    if ( !e.match(ExceptionC_ID, ExceptionC_Name) )
    {
      fmt_ABORT("exception mismatch: %s", e.toString().c_str());
    }
    dbg_PRINT("MAIN: p12ac");
    if (typeid(e) != typeid(RuntimeException))
    {
      fmt_ABORT("exception not RuntimeException but `%s`", typeid(e).name()); // ERROR-2, if USE_RETHROW_EXCEPTION
    }

    void *retVal;
    if (pthread_join(thread, &retVal) != 0)
    {
      src_ABORT();
    }
    if (retVal != &tagSuccess)
    {
      src_ABORT();
    }

    if (CookieAValue != cookie1a || CookieBValue != cookie1b) {
      fmt_ABORT("cookie 0x%xu, 0x%xu", cookie1a, cookie1b);
    }
    if (CookieAValue != cookie2a || CookieBValue != cookie2b) {
      fmt_ABORT("cookie 0x%xu, 0x%xu", cookie2a, cookie2b);
    }

    e.cookiesAssert(__LINE__);
    dbg_PRINT("MAIN: p12: %s, id `%s`, hash 0x%zx", e.toString().c_str(), typeid(e).name(), typeid(e).hash_code());
    if ( !e.match(ExceptionC_ID, ExceptionC_Name) )
    {
      fmt_ABORT("exception mismatch: %s", e.toString().c_str());
    }
    dbg_PRINT("MAIN: p12c");
    if (typeid(e) != typeid(RuntimeException))
    {
      fmt_ABORT("exception not RuntimeException but `%s`", typeid(e).name()); // ERROR-2, if USE_RETHROW_EXCEPTION
    }
  } catch (...) {
    std::exception_ptr eptr = std::current_exception();
    util::handle_exception(eptr, E_FILE_LINE);
    src_ABORT();
  }
  dbg_PRINT("MAIN: XXX");

  return 0;
}

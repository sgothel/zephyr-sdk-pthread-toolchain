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

#include "zephyr/ztest_assert.h"
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include <pthread.h>
#include <stdexcept>
#include <exception>
#include <cstdint>
#include <cstring>
#include <cstdio>

#define E_FILE_LINE "test", __LINE__
// #define dbg_PRINT(fmt, ...)
#define dbg_PRINT(fmt, ...)                                                                        \
	{                                                                                          \
		fprintf(stderr, "DBG @ cpp_exceptions:%d %s: ", __LINE__, __func__);               \
		fprintf(stderr, (fmt)__VA_OPT__(, ) __VA_ARGS__);                                  \
		fputs("\n", stderr);                                                               \
	}

namespace
{

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cv = PTHREAD_COND_INITIALIZER;

enum class State {
	WaitingForThread2_ProceedTo_A,
	Thread2_Reached_A,

	WaitingForThread2_ProceedTo_B,
	Thread2_Reached_B,

	WaitingForThread2_Terminate
};

// "m" required. "cv" for signalling change.
static State state;

static uint8_t const tagSuccess = 42U;

// If we make things depending on this being 13, then we can prevent some compiler optimizations
static uint8_t volatile thirteen = 13U;

static void WaitState(State const waitingFor)
{
	zassert_ok(pthread_mutex_lock(&m));
	dbg_PRINT("WaitState.0: %d, wait %d", (int)state, (int)waitingFor);

	while (state != waitingFor) {
		zassert_ok(pthread_cond_wait(&cv, &m));
	}
	dbg_PRINT("WaitState.X: %d", (int)state);

	zassert_ok(pthread_mutex_unlock(&m));
}

static void SetStateSignalAndWait(State const next, State const waitingFor)
{
	zassert_ok(pthread_mutex_lock(&m));

	dbg_PRINT("SetStateSignalAndWait.0: %d -> %d, wait %d", (int)state, (int)next,
		  (int)waitingFor);
	state = next;
	zassert_ok(pthread_cond_signal(&cv));

	while (state != waitingFor) {
		zassert_ok(pthread_cond_wait(&cv, &m));
	}
	dbg_PRINT("SetStateSignalAndWait.X: %d", (int)state);

	zassert_ok(pthread_mutex_unlock(&m));
}

/// This function is a possible cancellation point and therefore marked with noexcept (not marked
/// with __THROW like ::fprintf).
inline __attribute__((always_inline)) static bool
handle_exception(std::exception_ptr eptr, const char *file, int line) noexcept
{ // NOLINT(performance-unnecessary-value-param) passing by value is OK
	if (eptr) {
		try {
			std::rethrow_exception(eptr);
		} catch (const std::exception &e) {
			::fprintf(stderr, "Exception caught @ %s:%d: %s\n", file, line, e.what());
			return true;
		}
	}
	return false;
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
 * see [GCC libstdc++ exceptions
 * manual](https://gcc.gnu.org/onlinedocs/libstdc++/manual/using_exceptions.html): <pre> size of the
 * pool will be sufficient for NUM exceptions of 6 * sizeof(void*) bytes, plus another NUM
 * exceptions captured in std::exception_ptr and rethrown using std::rethrow_exception.
 * </pre>
 */
class RuntimeException: public std::logic_error
{
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
	RuntimeException(short id, const char *msg, const char *, int line) noexcept
		: std::logic_error(msg), m_id(id), m_line((short)line), m_thread(pthread_self())
	// , m_file(file)
	{
#if LOCAL_EXCEPTION_MESSAGE
		strncpy(m_msg, msg, sizeof(m_msg) - 1);
		m_msg[sizeof(m_msg) - 1] = 0; // EOS
#endif
	}

	~RuntimeException() noexcept override = default; // virtual
	RuntimeException(const RuntimeException &o) noexcept = default;
	RuntimeException(RuntimeException &&o) noexcept = default;
	RuntimeException &operator=(const RuntimeException &o) noexcept = default;
	RuntimeException &operator=(RuntimeException &&o) noexcept = default;

	const char *message() const noexcept
	{
#if LOCAL_EXCEPTION_MESSAGE
		return m_msg;
#else
		return std::logic_error::what();
#endif
	}
	const char *what() const noexcept override
	{
		return message();
	}

	constexpr int id() const noexcept
	{
		return m_id;
	}
	constexpr size_t thread() const noexcept
	{
		return m_thread;
	}
	constexpr const char *file() const noexcept
	{
		return "file";
	} // m_file; }
	constexpr int line() const noexcept
	{
		return m_line;
	}

	/** Returns whole description to new string */
	std::string toString() const noexcept
	{
		char buffer[200];
		snprintf(buffer, sizeof(buffer),
			 "REx[id %hd, a %p, ck %d, src[t %zx @ `%s`:%hd]]: `%s`", m_id, this,
			 checkCookies(), thread(), file(), m_line, message());
		return std::string(buffer);
	}
	bool checkCookies() const noexcept
	{
		return CookieAValue == cookie1a && CookieAValue == cookie2a;
	}
	void cookiesAssert(int line) const
	{
		if (CookieAValue != cookie1a || CookieAValue != cookie2a) {
			dbg_PRINT("Potentially garbage: REx[id %hd, a %p, src[t %zx @ line %hd]]",
				  m_id, this, thread(), m_line);
		}
		zassert_equal(CookieAValue, cookie1a);
		zassert_equal(CookieAValue, cookie2a);
	}
	bool match(int id) const noexcept
	{
		return m_id == id;
	}
	bool match(const char *msg) const noexcept
	{
		size_t message_len = strnlen(message(), ExceptionNameLen);
		size_t msg_len = strnlen(msg, ExceptionNameLen);
		if (message_len != msg_len) {
			return false;
		}
		return 0 == strncmp(message(), msg, msg_len);
	}
	bool match(int id, const char *msg) const noexcept
	{
		return match(id) && match(msg);
	}

	std::ostream &operator<<(std::ostream &out) noexcept
	{
		return out << toString();
	}
};

ZTEST(cpp_exceptions, test_cpp_exceptions00)
{
	{
		dbg_PRINT("Size: RuntimeException %zu (/ %zu sizeof(void*) = %zu)]",
			  sizeof(RuntimeException), sizeof(void *),
			  sizeof(RuntimeException) / sizeof(void *));
		dbg_PRINT("Size: std::logic_error %zu (/ %zu sizeof(void*) = %zu)]",
			  sizeof(std::logic_error), sizeof(void *),
			  sizeof(std::logic_error) / sizeof(void *));
	}
	std::logic_error e0 = std::logic_error(ExceptionB_Name);
	std::invalid_argument e1 = std::invalid_argument(ExceptionC_Name);
	RuntimeException e2(ExceptionA_ID, ExceptionA_Name, E_FILE_LINE);
	RuntimeException e3(ExceptionB_ID, ExceptionB_Name, E_FILE_LINE);
	{
		std::exception const &e = e0;
		dbg_PRINT("p0a: what `%s`, id `%s`, hash 0x%zx", e.what(), typeid(e).name(),
			  typeid(e).hash_code());
		zassert_equal(0, strncmp(e.what(), ExceptionB_Name, ExceptionNameLen));
		zassert_equal(typeid(e), typeid(std::logic_error));
	}
	{
		std::exception const &e = e1;
		dbg_PRINT("p0b: what `%s`, id `%s`, hash 0x%zx", e.what(), typeid(e).name(),
			  typeid(e).hash_code());
		zassert_equal(0, strncmp(e.what(), ExceptionC_Name, ExceptionNameLen));
		zassert_equal(typeid(e), typeid(std::invalid_argument));
	}
	{
		RuntimeException const &e = e2;
		dbg_PRINT("p0c: %s, id `%s`, hash 0x%zx", e.toString().c_str(), typeid(e).name(),
			  typeid(e).hash_code());
		zassert_true(e.match(ExceptionA_ID, ExceptionA_Name));
		zassert_equal(typeid(e), typeid(RuntimeException));
	}
	{
		RuntimeException const &e = e3;
		dbg_PRINT("p0d: %s, id `%s`, hash 0x%zx", e.toString().c_str(), typeid(e).name(),
			  typeid(e).hash_code());
		zassert_true(e.match(ExceptionB_ID, ExceptionB_Name));
		zassert_equal(typeid(e), typeid(RuntimeException));
	}
	volatile unsigned int cookie1a = CookieAValue;
	volatile unsigned int cookie1b = CookieBValue;
	try {
		try {
			{
				const pthread_t self = pthread_self();
				dbg_PRINT("p0e 0x%p", (void*)(size_t)self); // NOLINT
				zassert_not_equal(0, self);
				throw RuntimeException(ExceptionB_ID, ExceptionB_Name, E_FILE_LINE);
			};
		} catch (RuntimeException const &e) {
			volatile unsigned int cookie2a = CookieAValue;
			volatile unsigned int cookie2b = CookieBValue;
			e.cookiesAssert(__LINE__);
			dbg_PRINT("p0f: %s, id `%s`, hash 0x%zx", e.toString().c_str(),
				  typeid(e).name(), typeid(e).hash_code());
			zassert_true(e.match(ExceptionB_ID, ExceptionB_Name));
			dbg_PRINT("p0g");
			zassert_equal(typeid(e), typeid(RuntimeException));
			dbg_PRINT("p0h");

			zassert_equal(CookieAValue, cookie1a);
			zassert_equal(CookieBValue, cookie1b);
			zassert_equal(CookieAValue, cookie2a);
			zassert_equal(CookieBValue, cookie2b);

			e.cookiesAssert(__LINE__);
			dbg_PRINT("p0i: %s, id `%s`, hash 0x%zx", e.toString().c_str(),
				  typeid(e).name(), typeid(e).hash_code());
			zassert_true(e.match(ExceptionB_ID, ExceptionB_Name));
			dbg_PRINT("p0j");
			zassert_equal(typeid(e), typeid(RuntimeException));

			dbg_PRINT("p0k");
			throw RuntimeException(ExceptionC_ID, ExceptionC_Name, E_FILE_LINE);
		} catch (...) {
			std::exception_ptr eptr = std::current_exception();
			handle_exception(eptr, E_FILE_LINE);
			zassert_unreachable();
		}
		zassert_unreachable();
	} catch (RuntimeException const &e) {
		volatile unsigned int cookie2a = CookieAValue;
		volatile unsigned int cookie2b = CookieBValue;
		e.cookiesAssert(__LINE__);
		dbg_PRINT("p0l: %s, id `%s`, hash 0x%zx", e.toString().c_str(),
			  typeid(e).name(), typeid(e).hash_code());
		zassert_true(e.match(ExceptionC_ID, ExceptionC_Name));
		dbg_PRINT("p0m");
		zassert_equal(typeid(e), typeid(RuntimeException));

		SetStateSignalAndWait(State::WaitingForThread2_Terminate,
				      State::WaitingForThread2_Terminate);

		zassert_equal(CookieAValue, cookie1a);
		zassert_equal(CookieBValue, cookie1b);
		zassert_equal(CookieAValue, cookie2a);
		zassert_equal(CookieBValue, cookie2b);

		e.cookiesAssert(__LINE__);
		dbg_PRINT("p0n: %s, id `%s`, hash 0x%zx", e.toString().c_str(),
			  typeid(e).name(), typeid(e).hash_code());
		zassert_true(e.match(ExceptionC_ID, ExceptionC_Name));
		dbg_PRINT("p0o");
		zassert_equal(typeid(e), typeid(RuntimeException));

		zassert_equal(CookieAValue, cookie1a);
		zassert_equal(CookieBValue, cookie1b);
		zassert_equal(CookieAValue, cookie2a);
		zassert_equal(CookieBValue, cookie2b);

		e.cookiesAssert(__LINE__);
		dbg_PRINT("p0p: %s, id `%s`, hash 0x%zx", e.toString().c_str(),
			  typeid(e).name(), typeid(e).hash_code());
		zassert_true(e.match(ExceptionC_ID, ExceptionC_Name));
		dbg_PRINT("p0q");
		zassert_equal(typeid(e), typeid(RuntimeException));
		dbg_PRINT("p0r XXX");
		return;
	} catch (...) {
		std::exception_ptr eptr = std::current_exception();
		handle_exception(eptr, E_FILE_LINE);
		zassert_unreachable();
	}
	zassert_unreachable();
}

static void Thread2Inner()
{
	try {
		dbg_PRINT("T2i: p1");
		{
			// Prevent any prediction and optimization by the compiler. "thirteen" is of
			// course 13, but it is volatile.
			if (thirteen == 13U) {
				dbg_PRINT("T2i: p2c"); // NOLINT
				const pthread_t self = pthread_self();
				dbg_PRINT("T2i: p2d %p", (void*)(size_t)self); // NOLINT
				zassert_not_equal(0, self);
				throw RuntimeException(ExceptionA_ID, ExceptionA_Name, E_FILE_LINE);
			}
		};
	} catch (RuntimeException const &e) {
		// ###########
		// # Point A #
		// ###########
		[[maybe_unused]] std::exception_ptr eptr;
		eptr = std::current_exception();
		e.cookiesAssert(__LINE__);
		dbg_PRINT("T2i: p3: %s, id `%s`, hash 0x%zx", e.toString().c_str(),
			  typeid(e).name(), typeid(e).hash_code());
		zassert_true(e.match(ExceptionA_ID, ExceptionA_Name));
		zassert_equal(typeid(e), typeid(RuntimeException));

		SetStateSignalAndWait(State::Thread2_Reached_A,
				      State::WaitingForThread2_ProceedTo_B);

		e.cookiesAssert(__LINE__);
		dbg_PRINT("T2i: p4: %s, id `%s`, hash 0x%zx", e.toString().c_str(),
			  typeid(e).name(), typeid(e).hash_code());
		zassert_true(e.match(ExceptionA_ID, ExceptionA_Name));
		zassert_equal(typeid(e), typeid(RuntimeException));

		dbg_PRINT("T2i: p5");
		std::rethrow_exception(eptr);
		// throw; // ERROR-1
	} catch (...) {
		std::exception_ptr eptr = std::current_exception();
		handle_exception(eptr, E_FILE_LINE);
		zassert_unreachable();
	}
	zassert_unreachable();
}

static void *Thread2Entry(void *)
{
	dbg_PRINT("START: Thread2");

	try {
		Thread2Inner();
	} catch (RuntimeException const &e) {
		// ###########
		// # Point B #
		// ###########
		e.cookiesAssert(__LINE__);
		dbg_PRINT("T2e: p3: %s, id `%s`, hash 0x%zx", e.toString().c_str(),
			  typeid(e).name(), typeid(e).hash_code());

		zassert_true(e.match(ExceptionA_ID, ExceptionA_Name));
		zassert_equal(typeid(e), typeid(RuntimeException));

		SetStateSignalAndWait(State::Thread2_Reached_B, State::WaitingForThread2_Terminate);

		e.cookiesAssert(__LINE__);
		dbg_PRINT("T2e: p4: %s, id `%s`, hash 0x%zx", e.toString().c_str(),
			  typeid(e).name(), typeid(e).hash_code());

		zassert_true(e.match(ExceptionA_ID, ExceptionA_Name));
		zassert_equal(typeid(e), typeid(RuntimeException));
		dbg_PRINT("T2e: XXX OK");
		return (void *)&tagSuccess;
	} catch (...) {
		std::exception_ptr eptr = std::current_exception();
		handle_exception(eptr, E_FILE_LINE);
		zassert_true(false);
	}
	zassert_true(false);
	return nullptr;
}

} // anonymous namespace

ZTEST(cpp_exceptions, test_cpp_exceptions01)
{
	dbg_PRINT("MAIN: START");

	SetStateSignalAndWait(State::WaitingForThread2_ProceedTo_A,
			      State::WaitingForThread2_ProceedTo_A);

	dbg_PRINT("MAIN: p0b");
	pthread_t thread;
	zassert_ok(pthread_create(&thread, nullptr, &Thread2Entry, nullptr));

	dbg_PRINT("MAIN: p1");
	WaitState(State::Thread2_Reached_A);

	volatile unsigned int cookie1a = CookieAValue;
	volatile unsigned int cookie1b = CookieBValue;
	try {
		try {
			dbg_PRINT("MAIN: p2a");
			{
				const pthread_t self = pthread_self();
				dbg_PRINT("MAIN: p2b 0x%p", (void*)(size_t)self);
				zassert_not_equal(0, self);
			}
			{
				// Prevent any prediction and optimization by the compiler.
				// "thirteen" is of course 13, but it is volatile.
				if (thirteen == 13U) {
					dbg_PRINT("MAIN: p2c"); // NOLINT
					const pthread_t self = pthread_self();
					dbg_PRINT("MAIN: p2d 0x%p", (void*)(size_t)self); // NOLINT
					zassert_not_equal(0, self);
					throw RuntimeException(ExceptionB_ID, ExceptionB_Name,
							       E_FILE_LINE);
				}
			};
		} catch (RuntimeException const &e) {
			volatile unsigned int cookie2a = CookieAValue;
			volatile unsigned int cookie2b = CookieBValue;
			e.cookiesAssert(__LINE__);
			dbg_PRINT("MAIN: p3: %s, id `%s`, hash 0x%zx", e.toString().c_str(),
				  typeid(e).name(), typeid(e).hash_code());
			zassert_true(e.match(ExceptionB_ID, ExceptionB_Name));
			dbg_PRINT("MAIN: p3c");
			zassert_equal(typeid(e), typeid(RuntimeException));
			dbg_PRINT("MAIN: p3d");

			SetStateSignalAndWait(State::WaitingForThread2_ProceedTo_B,
					      State::Thread2_Reached_B); // Corrupts 'e'
			dbg_PRINT("MAIN: p3e");

			zassert_equal(CookieAValue, cookie1a);
			zassert_equal(CookieBValue, cookie1b);
			zassert_equal(CookieAValue, cookie2a);
			zassert_equal(CookieBValue, cookie2b);

			e.cookiesAssert(__LINE__);
			dbg_PRINT("MAIN: p4: %s, id `%s`, hash 0x%zx", e.toString().c_str(),
				  typeid(e).name(), typeid(e).hash_code());
			zassert_true(e.match(ExceptionB_ID, ExceptionB_Name));
			dbg_PRINT("MAIN: p4b");
			zassert_equal(typeid(e), typeid(RuntimeException));

			dbg_PRINT("MAIN: p5");
			throw RuntimeException(ExceptionC_ID, ExceptionC_Name, E_FILE_LINE);
		} catch (...) {
			std::exception_ptr eptr = std::current_exception();
			handle_exception(eptr, E_FILE_LINE);
			zassert_unreachable();
		}
		zassert_unreachable();
	} catch (RuntimeException const &e) {
		volatile unsigned int cookie2a = CookieAValue;
		volatile unsigned int cookie2b = CookieBValue;
		e.cookiesAssert(__LINE__);
		dbg_PRINT("MAIN: p11: %s, id `%s`, hash 0x%zx", e.toString().c_str(),
			  typeid(e).name(), typeid(e).hash_code());
		zassert_true(e.match(ExceptionC_ID, ExceptionC_Name));
		dbg_PRINT("MAIN: p11c");
		zassert_equal(typeid(e), typeid(RuntimeException));

		SetStateSignalAndWait(State::WaitingForThread2_Terminate,
				      State::WaitingForThread2_Terminate);

		zassert_equal(CookieAValue, cookie1a);
		zassert_equal(CookieBValue, cookie1b);
		zassert_equal(CookieAValue, cookie2a);
		zassert_equal(CookieBValue, cookie2b);

		e.cookiesAssert(__LINE__);
		dbg_PRINT("MAIN: p12a: %s, id `%s`, hash 0x%zx", e.toString().c_str(),
			  typeid(e).name(), typeid(e).hash_code());
		zassert_true(e.match(ExceptionC_ID, ExceptionC_Name));
		dbg_PRINT("MAIN: p12ac");
		zassert_equal(typeid(e), typeid(RuntimeException));

		void *retVal;
		zassert_ok(pthread_join(thread, &retVal));
		zassert_equal_ptr(retVal, &tagSuccess);

		zassert_equal(CookieAValue, cookie1a);
		zassert_equal(CookieBValue, cookie1b);
		zassert_equal(CookieAValue, cookie2a);
		zassert_equal(CookieBValue, cookie2b);

		e.cookiesAssert(__LINE__);
		dbg_PRINT("MAIN: p12: %s, id `%s`, hash 0x%zx", e.toString().c_str(),
			  typeid(e).name(), typeid(e).hash_code());
		zassert_true(e.match(ExceptionC_ID, ExceptionC_Name));
		dbg_PRINT("MAIN: p12c");
		zassert_equal(typeid(e), typeid(RuntimeException));
		dbg_PRINT("MAIN: XXX");
		return;
	} catch (...) {
		std::exception_ptr eptr = std::current_exception();
		handle_exception(eptr, E_FILE_LINE);
		zassert_unreachable();
	}
	zassert_unreachable();
}

static void before(void *arg)
{
	ARG_UNUSED(arg);

	dbg_PRINT("CONFIG_MULTITHREADING %d", IS_ENABLED(CONFIG_MULTITHREADING));
	dbg_PRINT("CONFIG_SMP %d", IS_ENABLED(CONFIG_SMP));
	dbg_PRINT("CONFIG_64BIT %d", IS_ENABLED(CONFIG_64BIT));
	dbg_PRINT("CONFIG_ARCH %s", CONFIG_ARCH);
	dbg_PRINT("[arm %d, arm64 %d, riscv %d, x86 %d]", 
		IS_ENABLED(CONFIG_ARM),
		IS_ENABLED(CONFIG_ARM64),
		IS_ENABLED(CONFIG_RISCV),
		IS_ENABLED(CONFIG_X86));

	if (!IS_ENABLED(CONFIG_DYNAMIC_THREAD)) {
		/* skip redundant testing if there is no thread pool / heap allocation */
		ztest_test_skip();
	}
	if (false && IS_ENABLED(CONFIG_X86)) {
		/* x86_64 toolchain issues w/ catching exceptions on a pthread */
		ztest_test_skip();
	}
}

ZTEST_SUITE(cpp_exceptions, NULL, NULL, before, NULL, NULL);

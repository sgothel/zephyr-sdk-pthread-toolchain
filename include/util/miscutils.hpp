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

#ifndef MISCUTILS_HPP_
#define MISCUTILS_HPP_

#include <cerrno>
#include <cstdint>
#include <cstdarg>
#include <cstring>
#include <cstdio>

#include <string>
#include <exception>

namespace util {
    /// This function is a possible cancellation point and therefore marked with noexcept (not marked with __THROW like ::fprintf).
    inline __attribute__((always_inline))
    static bool handle_exception(std::exception_ptr eptr, const char* file, int line) noexcept { // NOLINT(performance-unnecessary-value-param) passing by value is OK
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

    namespace lang {

        #if defined(__clang__)
            #define __attrdecl_no_optimize__ __attribute__ ((optnone))
            #define __attrdef_no_optimize__ __attribute__ ((optnone))
        #elif defined(__GNUC__) && !defined(__clang__)
            #define __attrdecl_no_optimize__ __attribute__((optimize("O0")))
            // #define __attrdecl_no_optimize__ [[gnu::optimize("O0")]]
            #define __attrdef_no_optimize__
        #else
            #define __attrdecl_no_optimize__
            #define __attrdef_no_optimize__
        #endif

        /**
         * Simple unary function wrapper which ensures function call to happen in order and not optimized away.
         */
        template <typename UnaryFunc>
        inline void callNotOptimize(UnaryFunc f) __attrdef_no_optimize__ {
            // asm asm-qualifiers ( AssemblerTemplate : OutputOperands [ : InputOperands [ : Clobbers ] ] )
            asm volatile("" : "+r,m"(f) : : "memory"); // a nop asm, usually guaranteeing synchronized order and non-optimization
            f();
        }
    } // util::lang

    namespace os {
        size_t CurrentThreadStackSpace();
    }

    namespace unsafe {
        /**
         * Returns a (potentially truncated) string according to `snprintf()` formatting rules
         * and variable number of arguments following the `format` argument
         * while utilizing the unsafe `vsnprintf`.
         *
         * This variant doesn't validate `format` against given arguments, see jau::format_string_n.
         *
         * Resulting string is truncated to `min(maxStrLen, formatLen)`,
         * with `formatLen` being the given formatted string length of output w/o limitation.
         *
         * @param maxStrLen maximum resulting string length
         * @param format `printf()` compliant format string
         * @param args optional arguments matching the format string
         */
        std::string format_string_n(const std::size_t maxStrLen, const char* format, ...) noexcept;
        std::string vformat_string_n(const std::size_t maxStrLen, const char* format, va_list args) noexcept;

        /**
         * Returns a (non-truncated) string according to `snprintf()` formatting rules
         * and variable number of arguments following the `format` argument
         * while utilizing the unsafe `vsnprintf`.
         *
         * This variant doesn't validate `format` against given arguments, see jau::format_string_h.
         *
         * Resulting string size matches formated output w/o limitation.
         *
         * @param strLenHint initially used string length w/o EOS
         * @param format `printf()` compliant format string
         * @param args optional arguments matching the format string
         */
        std::string format_string_h(const std::size_t strLenHint, const char* format, ...) noexcept;
        std::string vformat_string_h(const std::size_t strLenHint, const char* format, va_list args) noexcept;

        /// This function is a possible cancellation point and therefore marked with noexcept (not marked with __THROW like ::fprintf).
        void dbgPrint(FILE *out, const char *msg, bool addErrno, const char *func, const char *file, const int line,
                      const char* format, ...) noexcept;

    } // util::unsafe
} // util

#ifndef THIS_FILE
    #define THIS_FILE "test"
#endif
#define E_FILE_LINE THIS_FILE, __LINE__

#define dbg_PRINT(fmt, ...) { util::unsafe::dbgPrint(stderr, "DBG", false, __func__, THIS_FILE, __LINE__, (fmt) __VA_OPT__(,) __VA_ARGS__); }
#define fmt_ABORT(fmt, ...) { util::unsafe::dbgPrint(stderr, "ABORT", false, __func__, THIS_FILE, __LINE__, (fmt) __VA_OPT__(,) __VA_ARGS__); ::fflush(stderr); abort(); }
#define src_ABORT() { ::fprintf(stderr, "ABORT @ %s:%d %s: errno %d '%s'\n", THIS_FILE, __LINE__, __func__, errno, ::strerror(errno)); ::fflush(stderr); abort(); }

#endif // MISCUTILS_HPP_

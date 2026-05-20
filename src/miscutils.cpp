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

#include <util/miscutils.hpp>
#include <pthread.h>

#ifdef __ZEPHYR__
  #include <zephyr/kernel.h>
#endif

size_t util::os::CurrentThreadStackSpace() {
    size_t free_stack;
    #ifdef __ZEPHYR__
        struct k_thread *current_thread = k_current_get();
        if (k_thread_stack_space_get(current_thread, &free_stack)) {
            free_stack = 0;
        }
    #else
        free_stack = 1;
    #endif
    return free_stack;
}

std::string util::unsafe::vformat_string_n(const std::size_t maxStrLen, const char* format, va_list args) noexcept {
    std::exception_ptr eptr;
    std::string str;
    try {
        str.reserve(maxStrLen + 1);  // incl. EOS
        str.resize(maxStrLen);       // excl. EOS

        const size_t nchars = std::vsnprintf(&str[0], maxStrLen + 1, format, args); // NOLINT: clang-tidy bug
        if ( nchars < maxStrLen + 1 ) {
            str.resize(nchars);
            str.shrink_to_fit();
        }  // else truncated w/ nchars > MaxStrLen
    } catch (...) {
        eptr = std::current_exception();
    }
    handle_exception(eptr, E_FILE_LINE);
    return str;
}

std::string util::unsafe::format_string_n(const std::size_t maxStrLen, const char* format, ...) noexcept {
    va_list args;
    va_start (args, format);
    std::string str = vformat_string_n(maxStrLen, format, args);
    va_end (args);
    return str;
}

std::string util::unsafe::vformat_string_h(const std::size_t strLenHint, const char* format, va_list args) noexcept {
    std::exception_ptr eptr;
    size_t nchars;
    size_t bsz = strLenHint + 1;  // including EOS
    std::string str;
    try {
        str.reserve(bsz);  // incl. EOS
        str.resize(bsz-1); // excl. EOS

        nchars = std::vsnprintf(&str[0], bsz, format, args); // NOLINT: clang-tidy bug

        if( nchars < bsz ) {
            str.resize(nchars);
            str.shrink_to_fit();
        } else {
            bsz = std::min<size_t>(nchars+1, str.max_size()+1); // limit incl. EOS
            str.reserve(bsz);  // incl. EOS
            str.resize(bsz-1); // excl. EOS

            nchars = std::vsnprintf(&str[0], bsz, format, args); // NOLINT: clang-tidy bug

            str.resize(nchars);
        }
    } catch (...) {
        eptr = std::current_exception();
    }
    handle_exception(eptr, E_FILE_LINE);
    return str;
}

std::string util::unsafe::format_string_h(const std::size_t strLenHint, const char* format, ...) noexcept {
    va_list args;
    va_start (args, format);
    std::string str = vformat_string_h(strLenHint, format, args);
    va_end (args);
    return str;
}

void util::unsafe::dbgPrint(FILE *out, const char *msg, bool addErrno, const char *func, const char *file, const int line,
                               const char* format, ...) noexcept
{
    va_list args;
    va_start (args, format);
    std::exception_ptr eptr;
    try {
        ::fprintf(out, "[t %zx, s %4zu] %s @ %s:%d %s: ",
            pthread_self(), util::os::CurrentThreadStackSpace(), msg, file, line, func);
        ::fputs(vformat_string_n(256, format, args).c_str(), out);
        if( addErrno ) {
            ::fprintf(stderr, "; last errno %d %s", errno, strerror(errno));
        }
        fputs("\n", out);
        if( addErrno ) {
            ::fflush(stderr);
        }
    } catch (...) {
        eptr = std::current_exception();
    }
    handle_exception(eptr, E_FILE_LINE);
    va_end (args);
}

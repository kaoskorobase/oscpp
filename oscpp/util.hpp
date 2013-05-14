// oscpp library
//
// Copyright (c) 2004-2013 Stefan Kersten <sk@k-hornz.de>
//
// Permission is hereby granted, free of charge, to any person or organization
// obtaining a copy of the software and accompanying documentation covered by
// this license (the "Software") to use, reproduce, display, distribute,
// execute, and transmit the Software, and to prepare derivative works of the
// Software, and to permit third-parties to whom the Software is furnished to
// do so, all subject to the following:
//
// The copyright notices in the Software and this entire statement, including
// the above license grant, this restriction and the following disclaimer,
// must be included in all copies of the Software, in whole or in part, and
// all derivative works of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
// SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
// FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#ifndef OSCPP_UTIL_HPP_INCLUDED
#define OSCPP_UTIL_HPP_INCLUDED

#include <cassert>

namespace OSC
{
    static const size_t kAlignment = 4;

    inline bool isAligned(const void* ptr, size_t alignment)
    {
        return (reinterpret_cast<uintptr_t>(ptr) & (alignment-1)) == 0;
    }

    constexpr bool isAligned(size_t n)
    {
        return (n & 3) == 0;
    }

    constexpr size_t align(size_t n)
    {
        return (n + 3) & -4;
    }

    constexpr size_t padding(size_t n)
    {
        return align(n) - n;
    }

    inline void checkAlignment(const void* ptr, size_t n)
    {
        assert( isAligned(ptr, n) );
    }

    namespace Size
    {
        template <size_t N> constexpr size_t string(char const (&a)[N])
        {
            return align(N);
        }

        constexpr size_t array(size_t numElems)
        {
            // [numElems]
            return numElems+2;
        }

        template <size_t N> constexpr size_t message(char const (&address)[N], size_t numArgs)
        {
            return string(address) + align(numArgs+2);
        }

        constexpr size_t int32()
        {
            return 4;
        }

        constexpr size_t float32()
        {
            return 4;
        }

        constexpr size_t string(size_t n)
        {
            return align(n+1);
        }

        constexpr size_t blob(size_t n)
        {
            return 4 + align(n);
        }
    };
};

#endif // OSCPP_UTIL_HPP_INCLUDED
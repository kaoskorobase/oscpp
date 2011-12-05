// OSCpp library
//
// Copyright (c) 2004-2011 Stefan Kersten <sk@k-hornz.de>
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

#ifndef OSC_HOST_HPP_INCLUDED
#define OSC_HOST_HPP_INCLUDED

#include <boost/cstdint.hpp>
#include <boost/detail/endian.hpp>

#if defined(__GNUC__)
#   define oscpp_bswap32(x)  __builtin_bswap32(x)
#   define oscpp_bswap64(x)  __builtin_bswap64(x)
#elif defined(_WINDOWS_)
#   include <stdlib.h>
#   define oscpp_bswap32(x)  _byteswap_ulong(x)
#   define oscpp_bswap64(x)  _byteswap_uint64(x)
#else
    // Fallback implementation
    inline static int32_t oscpp_bswap32(int32_t l)
    {
        union u32 {
            uint8_t b[4];
            int32_t l;
        };
        u32 src, dst;
        src.l = l;
        dst.b[0] = src.b[3];
        dst.b[1] = src.b[2];
        dst.b[2] = src.b[1];
        dst.b[3] = src.b[0];
        return dst.l;
    }
    inline static int64_t oscpp_bswap64(int64_t l)
    {
        union u64 {
            uint8_t b[8];
            int64_t l;
        };
        u64 src, dst;
        src.l = l;
        dst.b[0] = src.b[7];
        dst.b[1] = src.b[6];
        dst.b[2] = src.b[5];
        dst.b[3] = src.b[4];
        dst.b[4] = src.b[3];
        dst.b[5] = src.b[2];
        dst.b[6] = src.b[1];
        dst.b[7] = src.b[0];
        return dst.l;
    }
#endif

namespace OSC
{
    inline static void convert32(void* v)
    {
#if defined(BOOST_LITTLE_ENDIAN)
        *static_cast<int32_t*>(v) = oscpp_bswap32(*static_cast<int32_t*>(v));
#endif
    }

    inline static void convert64(void* v)
    {
#if defined(BOOST_LITTLE_ENDIAN)
        *static_cast<int64_t*>(v) = oscpp_bswap64(*static_cast<int64_t*>(v));
#endif
    }
};

#endif // OSC_HOST_HPP_INCLUDED

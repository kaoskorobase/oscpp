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

#if !defined(OSC_HOST_BIG_ENDIAN) && !defined(OSC_HOST_LITTLE_ENDIAN)
#   if defined(_WINDOWS_)
#       define OSC_HOST_LITTLE_ENDIAN
        namespace OSC
        {
            inline static int32_t swap32(int32_t l)
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
        };
#   else
#       if defined(__APPLE__)
#           include <machine/endian.h>
#       else
#           include <endian.h>
#           include <netinet/in.h>
#       endif // __APPLE__
#       if BYTE_ORDER == BIG_ENDIAN
#           define OSC_HOST_BIG_ENDIAN
#           undef  OSC_HOST_LITTLE_ENDIAN
#       elif BYTE_ORDER == LITTLE_ENDIAN
#           define OSC_HOST_LITTLE_ENDIAN
#           undef  OSC_HOST_BIG_ENDIAN
#       else
#           error Unknown BYTE_ORDER
#       endif // BYTE_ORDER
        namespace OSC
        {
            inline static int32_t swap32(int32_t l)
            {
                return ntohl(l);
            }
        };
#   endif // _WINDOWS_
#endif

#if defined(OSC_HOST_BIG_ENDIAN) && defined(OSC_HOST_LITTLE_ENDIAN)
# error Only one of OSC_HOST_BIG_ENDIAN or OSC_HOST_LITTLE_ENDIAN can be defined!
#endif

namespace OSC
{
    inline static void convert32(void* v)
    {
#if defined(OSC_HOST_LITTLE_ENDIAN)
        *static_cast<int32_t*>(v) = swap32(*static_cast<int32_t*>(v));
#endif
    }

    inline static void convert64(void* v)
    {
#if defined(OSC_HOST_LITTLE_ENDIAN)
        int32_t tmp = static_cast<int32_t*>(v)[0];
        static_cast<int32_t*>(v)[0] = swap32(static_cast<int32_t*>(v)[1]);
        static_cast<int32_t*>(v)[1] = swap32(tmp);
#endif
    }
};

#endif // OSC_HOST_HPP_INCLUDED

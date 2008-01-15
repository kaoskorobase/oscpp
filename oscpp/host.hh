/*  -*- mode: c++; indent-tabs-mode: nil; c-basic-offset: 4 -*-
    vi: et sw=4:

    OSCpp Library. Copyright (c) 2004-2008 Stefan Kersten.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

#ifndef OSC_HOST_HH_INCLUDED
#define OSC_HOST_HH_INCLUDED

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

#endif // OSC_HOST_HH_INCLUDED

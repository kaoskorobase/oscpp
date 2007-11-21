/*  -*- mode: c++; indent-tabs-mode: nil; c-basic-offset: 4 -*-
    vi: et sw=4:

    OSC Template Library. Copyright (c) 2004, 2005 Stefan Kersten.

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

    $Id: OSC_Host.hh 8 2007-09-14 20:58:00Z sk $
*/

#ifndef OSC_HOST_HH_INCLUDED
#define OSC_HOST_HH_INCLUDED

#ifndef OSC_MAX_BUNDLE_NESTING
# define OSC_MAX_BUNDLE_NESTING 0
#endif

#if !defined(OSC_HOST_BIG_ENDIAN) && !defined(OSC_HOST_LITTLE_ENDIAN)
# error One of OSC_HOST_BIG_ENDIAN or OSC_HOST_LITTLE_ENDIAN must be defined!
#endif
#if defined(OSC_HOST_BIG_ENDIAN) && defined(OSC_HOST_LITTLE_ENDIAN)
# error Only one of OSC_HOST_BIG_ENDIAN or OSC_HOST_LITTLE_ENDIAN can be defined!
#endif

#ifdef OSC_HOST_LITTLE_ENDIAN
# include <netinet/in.h>
#endif

namespace OSC
{
    inline static void convert32(void* v)
    {
#ifdef OSC_HOST_LITTLE_ENDIAN
        *static_cast<int32_t*>(v) = ntohl(*static_cast<int32_t*>(v));
#endif
    }

    inline static void convert64(void* v)
    {
#ifdef OSC_HOST_LITTLE_ENDIAN
        int32_t tmp = static_cast<int32_t*>(v)[0];
        static_cast<int32_t*>(v)[0] = ntohl(static_cast<int32_t*>(v)[1]);
        static_cast<int32_t*>(v)[1] = ntohl(tmp);
#endif
    }
};

#endif // OSC_HOST_HH_INCLUDED

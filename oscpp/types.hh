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

#ifndef OSC_TYPES_HH_INCLUDED
#define OSC_TYPES_HH_INCLUDED

#include <stdint.h>

namespace OSC
{
    struct blob_t
    {
        size_t  size;
        void*   data;
    };
    
    typedef char byte_t;
};

#endif // OSC_TYPES_HH_INCLUDED

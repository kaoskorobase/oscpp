/*  -*- mode: c++; indent-tabs-mode: nil; c-basic-offset: 4 -*-
    vi: et sw=4:

    OSC Template Library. Copyright (c) 2004-2005 Stefan Kersten.

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

    $Id: OSC_Error.hh 4 2005-08-25 13:41:03Z sk $
*/

#ifndef OSC_ERROR_HH_INCLUDED
#define OSC_ERROR_HH_INCLUDED

#include <exception>
#include <string>

#ifdef NDEBUG
#  define M_OSC_DEBUG	0
#else
#  define M_OSC_DEBUG	1
#endif // OSC_DEBUG

#if M_OSC_DEBUG
#  define M_OSC_ASSERT(expr)                    \
        do {                                    \
            if (!(expr)) {                      \
                throw ::OSC::AssertionFailure();\
            }                                   \
        } while (0)
#else
#  define M_OSC_ASSERT(expr)
#endif // M_OSC_DEBUG

namespace OSC
{
    class Error : public std::exception
    {
    public:
        Error(const std::string& what)
            : m_what(what)
        { }

        ~Error() throw ()
        { }

        const char* what() throw ()
        {
            return m_what.c_str();
        }

    private:
        std::string m_what;
    }; // class Error

    class AssertionFailure : public Error
    {
    public:
        AssertionFailure()
            : Error(std::string("assertion failure"))
        { }
    };

    class UnderrunError : public Error
    {
    public:
        UnderrunError()
            : Error(std::string("buffer underrun"))
        { }
    };
    
    class OverflowError : public Error
    {
    public:
        OverflowError(size_t missing)
            : Error(std::string("buffer overflow")),
              m_missing(missing)
        { }

        size_t getMissing() const { return m_missing; }

    private:
        size_t m_missing;
    };
    
    class ParseError : public Error
    {
    public:
        ParseError()
            : Error(std::string("parse error"))
        { }
    };
};    

#endif // OSC_ERROR_HH_INCLUDED

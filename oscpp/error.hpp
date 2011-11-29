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

#ifndef OSC_ERROR_HPP_INCLUDED
#define OSC_ERROR_HPP_INCLUDED

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

#endif // OSC_ERROR_HPP_INCLUDED

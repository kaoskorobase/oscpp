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

    $Id: OSC_Stream.hh 5 2005-12-02 12:01:09Z sk $
*/

#ifndef OSC_STREAM_HH_INCLUDED
#define OSC_STREAM_HH_INCLUDED

#include "OSC_Error.hh"
#include "OSC_Host.hh"
#include "OSC_Types.hh"

#include <algorithm>
#include <cstring>

namespace OSC
{
    class Stream
    {
    public:
        Stream()
        {
            m_begin = m_end = m_pos = 0;
        }

        Stream(char* data, size_t size)
        {
            m_begin = data;
            m_end = m_begin + size;
            m_pos = m_begin;
        }

        Stream(const Stream& stream)
        {
            m_begin = m_pos = stream.m_pos;
            m_end = stream.m_end;
        }

        Stream(const Stream& stream, size_t size)
        {
            m_begin = m_pos = stream.m_pos;
            m_end = m_begin + size;
            if (m_end > stream.m_end) throw UnderrunError();
        }

        void reset()
        {
            m_pos = m_begin;
        }

        bool atEnd() const
        {
            return m_pos == m_end;
        }

        size_t getSize() const
        {
            return m_end - m_begin;
        }
            
        size_t getConsumed() const
        {
            return m_pos - m_begin;
        }

        size_t getConsumable() const
        {
            return m_end - m_pos;
        }

        char* getBegin() const
        {
            return m_begin;
        }

        char* getPos() const
        {
            return m_pos;
        }

        void setPos(char *pos)
        {
            m_pos = std::max(m_begin, std::min(m_end, pos));
        }

        static bool isAligned(size_t n)
        {
            return (n & 3) == 0;
        }

        static size_t getAligned(size_t n)
        {
            return (n + 3) & -4;
        }

        static size_t getPadding(size_t n)
        {
            return getAligned(n) - n;
        }

    protected:
        char*   m_begin;
        char*   m_end;
        char*   m_pos;
    };

    class WriteStream: public Stream
    {
    public:
        WriteStream()
            : Stream()
        { }

        WriteStream(char* data, size_t size)
            : Stream(data, size)
        { }

        WriteStream(const WriteStream& stream)
            : Stream(stream)
        { }

        WriteStream(const WriteStream& stream, size_t size)
            : Stream(stream, size)
        { }

        void checkWritable(size_t n) const throw (OverflowError)
        {
            if (getConsumable() < n) throw OverflowError(n - getConsumable());
        }

        void zero(size_t n) throw (OverflowError)
        {
            checkWritable(n);
            while (n--) *m_pos++ = '\0';
        }

        void putChar(char c) throw (OverflowError)
        {
            checkWritable(1);
            *m_pos++ = c;
        }

        void putInt32(int32_t v) throw (OverflowError)
        {
            checkWritable(4);
            *((int32_t*)m_pos) = v;
            convert32(m_pos);
            m_pos += 4;
        }

        void putUInt64(uint64_t v) throw (OverflowError)
        {
            checkWritable(8);
            *((uint64_t*)m_pos) = v;
            convert64(m_pos);
            m_pos += 8;
        }

        void putFloat32(float v) throw (OverflowError)
        {
            checkWritable(4);
            *((float*)m_pos) = v;
            convert32(m_pos);
            m_pos += 4;
        }

        void putData(const char* data, size_t size) throw (OverflowError)
        {
            size_t padding = Stream::getPadding(size);
            char* pos = m_pos;
            checkWritable(size+padding);
            memcpy(pos, data, size);
            memset(pos+size, 0, padding);
            m_pos = pos+size+padding;
        }

        void putString(const char* s) throw (OverflowError)
        {
            putData(s, strlen(s)+1);
        }
    };


    class ReadStream : public Stream
    {
    public:
        ReadStream()
            : Stream()
        { }

        ReadStream(char* data, size_t size)
            : Stream(data, size)
        { }

        ReadStream(const ReadStream& stream)
            : Stream(stream)
        { }

        ReadStream(const ReadStream& stream, size_t size)
            : Stream(stream, size)
        { }

        void checkReadable(size_t n) const throw (UnderrunError)
        {
            if (getConsumable() < n) throw UnderrunError();
        }

        void skip(size_t n) throw (UnderrunError)
        {
            checkReadable(n);
            m_pos += n;
        }

        char getChar() throw (UnderrunError)
        {
            checkReadable(1);
            return *m_pos++;
        }

        char peekChar() const throw (UnderrunError)
        {
            checkReadable(1);
            return *m_pos;
        }

        int32_t getInt32() throw (UnderrunError)
        {
            checkReadable(4);
            int32_t v = *(int32_t*)m_pos;
            m_pos += 4;
            convert32(&v);
            return v;
        }

        uint64_t getUInt64() throw (UnderrunError)
        {
            checkReadable(8);
            uint64_t v = *((uint64_t*)m_pos);
            m_pos += 8;
            convert64(&v);
            return v;
        }

        float getFloat32() throw (UnderrunError)
        {
            checkReadable(4);
            float v = *(float*)m_pos;
            m_pos += 4;
            convert32(&v);
            return v;
        }

        char* getString() throw (UnderrunError, ParseError)
        {
            char* ptr = m_pos;
            char* end = m_end;

            checkReadable(4); // min string length

            while (true) {
                if (ptr == end) throw UnderrunError();
                if (*ptr == '\0') break;
                ptr++;
            }

            size_t n = getPadding(++ptr - m_pos);
            while (n--) {
                if (ptr == end) throw UnderrunError();
                if (*ptr != '\0') throw ParseError();
                ptr++;
            }

            char* s = m_pos;
            m_pos = ptr;

            return s;
        }
    };
};

#endif // OSC_STREAM_HH_INCLUDED

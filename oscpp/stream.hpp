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

#ifndef OSC_STREAM_HPP_INCLUDED
#define OSC_STREAM_HPP_INCLUDED

#include <oscpp/error.hpp>
#include <oscpp/host.hpp>
#include <oscpp/types.hpp>

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

        Stream(void* data, size_t size)
        {
            m_begin = static_cast<byte_t*>(data);
            m_end   = m_begin + size;
            m_pos   = m_begin;
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

        void* getBegin() const
        {
            return m_begin;
        }

        void* getPos() const
        {
            return m_pos;
        }

        void setPos(void* pos)
        {
            m_pos = std::max(m_begin, std::min(m_end, static_cast<byte_t*>(pos)));
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
        byte_t* m_begin;
        byte_t* m_end;
        byte_t* m_pos;
    };

    class WriteStream: public Stream
    {
    public:
        WriteStream()
            : Stream()
        { }

        WriteStream(void* data, size_t size)
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

        void skip(size_t n) throw (OverflowError)
        {
            checkWritable(n);
            m_pos += n;
        }
        
        void zero(size_t n) throw (OverflowError)
        {
            checkWritable(n);
            memset(m_pos, 0, n);
            m_pos += n;
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

        void putData(const void* data, size_t size) throw (OverflowError)
        {
            size_t padding = Stream::getPadding(size);
            byte_t* pos = m_pos;
            checkWritable(size+padding);
            memcpy(pos,      data, size);
            memset(pos+size, 0,    padding);
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

        ReadStream(void* data, size_t size)
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
            char* ptr = static_cast<char*>(m_pos);
            char* end = static_cast<char*>(m_end);

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

#endif // OSC_STREAM_HPP_INCLUDED

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

#ifndef OSCPP_SERVER_HPP_INCLUDED
#define OSCPP_SERVER_HPP_INCLUDED

#include <oscpp/stream.hpp>
#include <oscpp/util.hpp>

#include <algorithm>
#include <cstdint>
#include <cstring>

namespace OSC
{
namespace Server
{
    class PacketTest
    {
    public:
        static bool isMessage(const void* data, size_t size)
        {
            return (size > 3) && (static_cast<const char*>(data)[0] != '#');
        }
        static bool isMessage(const ReadStream& stream)
        {
            return isMessage(stream.pos(), stream.consumable());
        }
        static bool isBundle(const void* data, size_t size)
        {
            return (size > 15) && (memcmp(data, "#bundle", 8) == 0);
        }
        static bool isBundle(const ReadStream& stream)
        {
            return isBundle(stream.pos(), stream.consumable());
        }
    };

    //! OSC Message Argument Iterator.
    /*!
     * Retrieve typed arguments from an incoming message.
     * 
     * Supported tags and their correspondong types are:
     *
     *  i       -- 32 bit signed integer number<br>
     *  f       -- 32 bit floating point number<br>
     *  s       -- NULL-terminated string padded to 4-byte boundary<br>
     *  b       -- 32-bit integer size followed by 4-byte aligned data
     * 
     * \sa getArgInt32
     * \sa getArgFloat32
     * \sa getArgString
     */
    class ArgStream
    {
    public:
        //! Constructor.
        /*!
         * Read arguments from stream, which has to point to the start of a
         * message type signature.
         *
         * \throw OSC::UnderrunError stream buffer underrun.
         * \throw OSC::ParseError error while parsing input stream.
         */
        ArgStream(const ReadStream& stream)
            : m_args(stream)
        {
            const char* tags = m_args.getString();
            if (tags[0] != ',') throw ParseError("Tag string doesn't start with ','");
            m_tags = ReadStream(tags+1, strlen(tags)-1);
        }

        //! Return size of stream.
        /*!
         * Return number of arguments that can be read from the stream
         * according to the message's type signature.
         */
        size_t getSize() const
        {
            return m_tags.capacity();
        }

        //! End of stream reached?
        /*!
         * Return true if no more arguments can be read from the input
         * stream according to the message's type signature.
         */
        bool atEnd() const
        {
            return m_tags.atEnd();
        }

        //! Get next type tag.
        /*!
         * Return the type tag corresponding to the next message argument.
         *
         * \throw OSC::UnderrunError stream buffer underrun.
         * \sa OSC::ArgIter
         */
        char tag() const
        {
            return m_tags.peekChar();
        }

        //* Drop next argument.
        void drop()
        {
            const char t = tag();
            switch (t) {
                case 'i': m_tags.skip(1); m_args.skip(4); break;
                case 'f': m_tags.skip(1); m_args.skip(4); break;
                case 's': m_tags.skip(1); m_args.getString(); break;
                case 'b': blob(); break;
                default: throw ParseError("Unknown type tag");
            }
        }

        //! Get next integer argument.
        /*!
         * Read next numerical argument from the input stream and convert it to
         * an integer.
         *
         * \exception OSC::UnderrunError stream buffer underrun.
         * \exception OSC::ParseError argument could not be converted.
         */
        int32_t int32()
        {
            const char t = m_tags.getChar();
            if (t == 'i') return m_args.getInt32();
            if (t == 'f') return (int32_t)m_args.getFloat32();
            throw ParseError("Cannot convert argument to int");
        }

        //! Get next float argument.
        /*!
         * Read next numerical argument from the input stream and convert it to
         * a float.
         *
         * \exception OSC::UnderrunError stream buffer underrun.
         * \exception OSC::ParseError argument could not be converted.
         */
        float float32()
        {
            const char t = m_tags.getChar();
            if (t == 'f') return m_args.getFloat32();
            if (t == 'i') return (float)m_args.getInt32();
            throw ParseError("Cannot convert argument to float");
        }

        //! Get next string argument.
        /*!
         * Read next string argument and return it as a NULL-terminated string.
         *
         * \exception OSC::UnderrunError stream buffer underrun.
         * \exception OSC::ParseError argument could not be converted or is not
         * a valid string.
         */
        const char* string()
        {
            if (m_tags.getChar() == 's') {
                return m_args.getString();
            }
            throw ParseError("Cannot convert argument to string");
        }

        //! Get next blob argument.
        /*!
         * Read next blob argument and return it as a NULL-terminated string.
         *
         * @throw OSC::UnderrunError stream buffer underrun.
         * @throw OSC::ParseError argument is not a valid blob
         */
        Blob blob()
        {
            if (m_tags.getChar() == 'b') {
                int32_t size = m_args.getInt32();
                if (size < 0) {
                    throw ParseError("Invalid blob size is less than zero");
                } else {
                    static_assert(sizeof(size_t) >= sizeof(int32_t),
                                  "Size of size_t must be greater than size of int32_t");
                    const void* data = m_args.pos();
                    m_args.skip(OSC::align(size));
                    return Blob { static_cast<size_t>(size)
                                , data };
                }
            } else {
                throw ParseError("Cannot convert argument to blob");
            }
        }

    private:
        ReadStream m_args;
        ReadStream m_tags;
    };

    class Message
    {
    public:
        Message(const char* address, const ReadStream& stream)
            : m_address(address)
            , m_args(ArgStream(stream))
        { }

        const char* address() const
        {
            return m_address;
        }

        const ArgStream& args() const
        {
            return m_args;
        }

    private:
        const char* m_address;
        ArgStream   m_args;
    };
    
    class PacketIterator;

    class Bundle
    {
    public:
        Bundle(uint64_t time, const ReadStream& stream)
            : m_time(time)
            , m_stream(stream)
        { }

        uint64_t time() const
        {
            return m_time;
        }

        PacketIterator begin() const;
        PacketIterator end() const;

    private:
        uint64_t   m_time;
        ReadStream m_stream;
    };

    class Packet
    {
    public:
        Packet()
            : m_isBundle(false)
        { }

        Packet(const ReadStream& stream)
            : m_stream(stream)
            , m_isBundle(PacketTest::isBundle(stream))
        {
            // Skip over #bundle header
            if (m_isBundle) m_stream.skip(8);
        }

        Packet(const void* data, size_t size)
            : Packet(ReadStream(data, size))
        { }

        bool isBundle() const
        {
            return m_isBundle;
        }

        bool isMessage() const
        {
            return !isBundle();
        }

        operator Bundle () const
        {
            if (!isBundle()) throw ParseError("Packet is not a bundle");
            ReadStream stream(m_stream);
            uint64_t time = stream.getUInt64();
            return Bundle(time, std::move(stream));
        }

        operator Message () const
        {
            if (!isMessage()) throw ParseError("Packet is not a message");
            ReadStream stream(m_stream);
            const char* address = stream.getString();
            return Message(address, std::move(stream));
        }

    private:
        ReadStream m_stream;
        bool       m_isBundle;
    };
    
    class PacketIterator
    {
    public:
        PacketIterator(const ReadStream& stream)
            : m_stream(stream)
            , m_skip(0)
        {
            if (!m_stream.atEnd())
                m_skip = getPacket();
        }

        PacketIterator& operator++()
        {
            m_stream.skip(m_skip);
            if (m_stream.atEnd())
                m_skip = 0;
            else
                m_skip = getPacket();
            return *this;
        }

        bool operator==(const PacketIterator& it)
        {
            return m_stream.pos() == it.m_stream.pos();
        }

        bool operator!=(const PacketIterator& it)
        {
            return m_stream.pos() != it.m_stream.pos();
        }

        const Packet& operator*()
        {
            if (m_stream.atEnd())
                throw ParseError("Trying to read past end of packet");
            return m_packet;
        }

    private:
        size_t getPacket()
        {
            const size_t size = m_stream.peekInt32();
            m_packet = Packet(ReadStream(m_stream, size));
            return sizeof(int32_t) + size;
        }

        ReadStream m_stream;
        Packet     m_packet;
        size_t     m_skip;
    };

    PacketIterator Bundle::begin() const
    {
        return PacketIterator(m_stream);
    }

    PacketIterator Bundle::end() const
    {
        return PacketIterator(ReadStream(m_stream.end(), 0));
    }
}; // namespace Server
}; // namespace OSC

static inline bool operator==(const OSC::Server::Message& msg, const char* str)
{
    return strcmp(msg.address(), str) == 0;
}

static inline bool operator==(const char* str, const OSC::Server::Message& msg)
{
    return msg == str;
}

#endif // OSCPP_SERVER_HPP_INCLUDED

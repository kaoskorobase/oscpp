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
        //* Empty argument stream.
        ArgStream() = default;

        //* Construct argument stream from tag and value streams.
        ArgStream(const ReadStream& tags, const ReadStream& args)
            : m_tags(tags)
            , m_args(args)
        { }

        //! Constructor.
        /*!
         * Read arguments from stream, which has to point to the start of a
         * message type signature.
         *
         * \throw OSC::UnderrunError stream buffer underrun.
         * \throw OSC::ParseError error while parsing input stream.
         */
        ArgStream(const ReadStream& stream)
        {
            m_args = stream;
            const char* tags = m_args.getString();
            if (tags[0] != ',') throw ParseError("Tag string doesn't start with ','");
            m_tags = ReadStream(tags+1, strlen(tags)-1);
        }

        //* Return the number of arguments that can be read from the stream.
        size_t size() const
        {
            return m_tags.capacity();
        }

        //* Return true if no more arguments can be read from the stream.
        bool atEnd() const
        {
            return m_tags.atEnd();
        }

        //* Return tag and argument streams.
        std::tuple<ReadStream,ReadStream> state() const
        {
            return std::make_tuple(m_tags, m_args);
        }

        //* Return the type tag corresponding to the next message argument.
        char tag() const
        {
            return m_tags.peekChar();
        }

        //* Drop next argument.
        void drop()
        {
            drop(m_tags.getChar());
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

        //* Get next blob argument.
        //
        // @throw OSC::UnderrunError stream buffer underrun.
        // @throw OSC::ParseError argument is not a valid blob
        Blob blob()
        {
            if (m_tags.getChar() == 'b') {
                return parseBlob();
            } else {
                throw ParseError("Cannot convert argument to blob");
            }
        }

        //* Return a stream corresponding to an array argument.
        ArgStream array()
        {
            if (m_tags.getChar() == '[') {
                const byte_t* tags = m_tags.pos();
                const byte_t* args = m_args.pos();
                dropArray();
                // m_tags.pos() points right after the closing ']'.
                return ArgStream(ReadStream(tags, m_tags.pos() - tags - 1),
                                 ReadStream(args, m_args.pos() - args));
            } else {
                throw ParseError("Expected array");
            }
        }

        template <typename T> T next()
        {
            return T::OSC_Server_ArgStream_next_unimplemented;
        }

    private:
        // Parse a blob (type tag already consumed).
        Blob parseBlob()
        {
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
        }
        // Drop an atomic value of type t (type tag already consumed).
        void dropAtom(char t)
        {
            switch (t) {
                case 'i': m_args.skip(4); break;
                case 'f': m_args.skip(4); break;
                case 's': m_args.getString(); break;
                case 'b': parseBlob(); break;
            }
        }
        // Drop a possibly nested array.
        void dropArray()
        {
            unsigned int level = 0;
            for (;;) {
                char t = m_tags.getChar();
                if (t == ']') {
                    if (level == 0) break;
                    else level--;
                } else if (t == '[') {
                    level++;
                } else {
                    dropAtom(t);
                }
            }
        }
        // Drop the next argument of type t (type tag already consumed).
        void drop(char t)
        {
            switch (t) {
                case '[': dropArray(); break;
                default: dropAtom(t);
            }
        }

    private:
        ReadStream m_tags;
        ReadStream m_args;
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

        ArgStream args() const
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

        inline PacketIterator begin() const;
        inline PacketIterator end() const;

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

        const void* data() const
        {
            return m_stream.begin();
        }

        size_t size() const
        {
            return m_stream.capacity();
        }

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
            ReadStream stream(m_stream);
            const int32_t size = stream.getInt32();
            if (size <= 0)
                throw ParseError("Invalid packet size");
            m_packet = Packet(ReadStream(stream, size));
            return sizeof(int32_t) + size;
        }

        ReadStream m_stream;
        Packet     m_packet;
        size_t     m_skip;
    };

    template <> inline int32_t ArgStream::next<int32_t>()
    {
        return int32();
    }

    template <> inline float ArgStream::next<float>()
    {
        return float32();
    }

    template <> inline const char* ArgStream::next<const char*>()
    {
        return string();
    }

    template <> inline Blob ArgStream::next<Blob>()
    {
        return blob();
    }

    template <> inline ArgStream ArgStream::next<ArgStream>()
    {
        return array();
    }

    PacketIterator Bundle::begin() const
    {
        return PacketIterator(m_stream);
    }

    PacketIterator Bundle::end() const
    {
        return PacketIterator(ReadStream(m_stream.end(), 0));
    }
} // namespace Server
} // namespace OSC

static inline bool operator==(const OSC::Server::Message& msg, const char* str)
{
    return strcmp(msg.address(), str) == 0;
}

static inline bool operator==(const char* str, const OSC::Server::Message& msg)
{
    return msg == str;
}

#endif // OSCPP_SERVER_HPP_INCLUDED

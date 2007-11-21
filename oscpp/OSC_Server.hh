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

    $Id: OSC_Server.hh 9 2007-09-14 20:58:40Z sk $
*/

#ifndef OSC_SERVER_HH_INCLUDED
#define OSC_SERVER_HH_INCLUDED

#include "OSC_Stream.hh"

#include <algorithm>
#include <cstring>

namespace OSC
{
    class PacketTest
    {
    public:
        static bool isMessage(const char *data, size_t size)
        {
            return (size > 3) && (*data != '#');
        }
        static bool isMessage(ReadStream& stream)
        {
            return isMessage(stream.getPos(), stream.getConsumable());
        }

        static bool isBundle(const char* data, size_t size)
        {
            return (size > 15) && (memcmp(data, "#bundle", 8) == 0);
        }
        static bool isBundle(ReadStream& stream)
        {
            return isBundle(stream.getPos(), stream.getConsumable());
        }
    };

    struct ServerPacket
    {
        ServerPacket(char* d, size_t s)
        {
            ReadStream stream(d, s);
            if (PacketTest::isBundle(stream)) {
                isBundle = true;
                stream.skip(8); // skip #bundle tag
                time = stream.getUInt64();
            } else if (PacketTest::isMessage(stream)) {
                isBundle = false;
                time = 1;
            } else {
                throw ParseError();
            }
            data = stream.getPos();
            size = stream.getConsumable();
        };

        bool        isBundle;
        uint64_t    time;
        char*       data;
        size_t      size;
    };
    
    struct Message
    {
        char* path;
        char* data;
        size_t size;
    };
    
    class MessageStream
    {
        static void next_m(ReadStream& stream, Message& m)
        {
            size_t size = stream.getConsumable();
            ReadStream mstream(stream);
            m.path = mstream.getString();
            m.data = mstream.getPos();
            m.size = mstream.getConsumable();
            stream.skip(size);
        }
        static void next_b(ReadStream& stream, Message& m)
        {
            size_t size = stream.getInt32();
            if (PacketTest::isMessage(stream)) {
                ReadStream mstream(stream, size);
                m.path = mstream.getString();
                m.data = mstream.getPos();
                m.size = mstream.getConsumable();
            }
            stream.skip(size);
        }

    public:
        MessageStream(ServerPacket& packet)
            : m_stream(packet.data, packet.size)
        {
            m_next = packet.isBundle ? &next_b : &next_m;
        }

        bool atEnd() const
        {
            return m_stream.atEnd();
        }
        
        Message next()
        {
            Message m;
            (*m_next)(m_stream, m);
            return m;
        }

    private:
        ReadStream  m_stream;
        void (*m_next)(ReadStream&, Message&);
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
    private:
        void initFromArgStream() throw(ParseError)
        {
            char* tagString = m_argStream.getString();
            if (*tagString++ != ',') throw ParseError();
            m_tagStream = ReadStream(tagString, strlen(tagString));
        }

    public:
        //! Constructor.
        /*!
         * Read arguments from stream, which has to point to the start of a
         * message type signature.
         *
         * \throw OSC::UnderrunError stream buffer underrun.
         * \throw OSC::ParseError error while parsing input stream.
         */
        ArgStream(char* data, size_t size) throw(ParseError)
            : m_argStream(data, size)
        {
            initFromArgStream();
        }
        ArgStream(Message& msg) throw(ParseError)
            : m_argStream(msg.data, msg.size)
        {
            initFromArgStream();
        }
        ArgStream(const ArgStream& other)
            : m_tagStream(other.m_tagStream),
							m_argStream(other.m_argStream)
        { }
        ArgStream()
        { }

        //! Reset stream to initial state.
        void reset()
        {
            m_argStream.reset();
            m_tagStream.reset();
        }
            
        //! Return size of stream.
        /*!
         * Return number of arguments that can be read from the stream
         * according to the message's type signature.
         */
        size_t getSize() const
        {
            return m_tagStream.getSize();
        }

        //! End of stream reached?
        /*!
         * Return true if no more arguments can be read from the input
         * stream according to the message's type signature.
         */
        bool atEnd() const
        {
            return m_tagStream.atEnd();
        }

        //! Get next type tag.
        /*!
         * Return the type tag corresponding to the next message argument.
         *
         * \throw OSC::UnderrunError stream buffer underrun.
         * \sa OSC::ArgIter
         */
        char peekTag() const throw (UnderrunError)
        {
            return m_tagStream.peekChar();
        }

        //! Get next integer argument.
        /*!
         * Read next numerical argument from the input stream and convert it to
         * an integer.
         *
         * \exception OSC::UnderrunError stream buffer underrun.
         * \exception OSC::ParseError argument could not be converted.
         */
        int32_t getInt32() throw (UnderrunError, ParseError)
        {
            register char t = m_tagStream.getChar();
            if (t == 'i') return m_argStream.getInt32();
            if (t == 'f') return (int32_t)m_argStream.getFloat32();
            throw ParseError();
        }

        //! Get next float argument.
        /*!
         * Read next numerical argument from the input stream and convert it to
         * a float.
         *
         * \exception OSC::UnderrunError stream buffer underrun.
         * \exception OSC::ParseError argument could not be converted.
         */
        float getFloat32() throw (UnderrunError, ParseError)
        {
            register char t = m_tagStream.getChar();
            if (t == 'f') return m_argStream.getFloat32();
            if (t == 'i') return (float)m_argStream.getInt32();
            throw ParseError();
        }

        //! Get next string argument.
        /*!
         * Read next string argument and return it as a NULL-terminated string.
         *
         * \exception OSC::UnderrunError stream buffer underrun.
         * \exception OSC::ParseError argument could not be converted or is not
         * a valid string.
         */
        const char* getString() throw (UnderrunError, ParseError)
        {
            if (m_tagStream.getChar() == 's') {
                char* res = m_argStream.getString();
                if (res) return res;
            }
            throw ParseError();
        }

        //! Get next blob argument.
        /*!
         * Read next blob argument and return it as a NULL-terminated string.
         *
         * \exception OSC::UnderrunError stream buffer underrun.
         * \exception OSC::ParseError argument could not be converted or is not
         * a valid string.
         */
        blob_t getBlob() throw (UnderrunError, ParseError)
        {
            blob_t blob;
            if (m_tagStream.getChar() == 'b') {
                blob.size = m_argStream.getInt32();
                if (blob.size >= 0) {
                    blob.data = static_cast<void*>(m_argStream.getPos());
                    m_argStream.skip(blob.size);
                } else {
                    throw ParseError();
                }
            } else {
                throw ParseError();
            }
            return blob;
        }

    private:
        ReadStream	m_tagStream;
        ReadStream	m_argStream;
    };
};

static inline bool operator==(const OSC::Message& msg, const char* str)
{
    return strcmp(msg.path, str) == 0;
}

static inline bool operator==(const char* str, const OSC::Message& msg)
{
    return msg == str;
}

#endif // OSC_SERVER_HH_INCLUDED

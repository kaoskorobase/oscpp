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

    $Id: OSC_Client.hh 6 2006-12-13 17:02:36Z sk $
*/

#ifndef OSC_CLIENT_HH_INCLUDED
#define OSC_CLIENT_HH_INCLUDED

#include "OSC_Host.hh"
#include "OSC_Stream.hh"

namespace OSC
{
    //! OSC packet construction.
    /*!
     * Contruct a valid OSC packet for transmitting over a transport medium.
     */
    class ClientPacket
    {
    public:
        //! Constructor.
        /*!
        */
        ClientPacket()
        {
            reset(0, 0);
        }

        //! Constructor.
        /*!
        */
        ClientPacket(char* buffer, size_t size)
        {
            reset(buffer, size);
        }

        //! Get packet buffer address.
        /*!
         * Return the start address of the packet currently under construction.
         */
        char* getData() const
        {
            return m_buffer;
        }

        size_t getBufferSize() const
        {
            return m_bufferSize;
        }

        //! Get packet content size.
        /*!
         * Return the size of the packet currently under construction.
         */
        size_t getSize() const
        {
            return m_argStream.getConsumed();
        }

        //! Reset packet state.
        void reset(char* buffer, size_t size)
        {
            m_buffer = buffer;
            m_bufferSize = size;
            m_argStream = WriteStream(m_buffer, m_bufferSize);
            m_sizePosM = m_sizePosB = 0;
            m_inBundle = 0;
        }

        void reset()
        {
            reset(m_buffer, m_bufferSize);
        }

        void openBundle(uint64_t time)
        {
            if (m_inBundle) {
                // get current stream pos
                byte_t* curPos = m_argStream.getPos();
                // remember previous size pos offset
                *(int32_t*)curPos = m_sizePosB - m_argStream.getBegin();
                m_argStream.skip(4);
                // record size pos
                m_sizePosB = curPos;
            }
            m_inBundle++;
            m_argStream.putString("#bundle");
            m_argStream.putUInt64(time);
        }

        void closeBundle()
        {
            if (m_inBundle > 0) {
                if (m_inBundle > 1) {
                    // get current stream pos
                    byte_t* curPos = m_argStream.getPos();
                    // get previous size pos
                    byte_t* prevPos = m_argStream.getBegin() + *(int32_t*)m_sizePosB;
                    // write bundle size
                    m_argStream.setPos(m_sizePosB);
                    m_argStream.putInt32(curPos - m_sizePosB - 4);
                    // restore stream pos
                    m_argStream.setPos(curPos);
                    // record outer bundle size pos
                    m_sizePosB = prevPos;
                }
                m_inBundle--;
            } else {
                throw UnderrunError();
            }
        }

        void openMessage(const char* addr, size_t numArgs)
        {
            if (m_inBundle) {
                // record message size pos
                m_sizePosM = m_argStream.getPos();
                // advance arg stream
                m_argStream.skip(4);
            }
            m_argStream.putString(addr);
            size_t sigLen = numArgs+2;
            m_tagStream = WriteStream(m_argStream, sigLen);
            m_argStream.zero(WriteStream::getAligned(sigLen));
            m_tagStream.putChar(',');
        }

        void closeMessage()
        {
            if (m_inBundle) {
                // get current stream pos
                byte_t* curPos = m_argStream.getPos();
                // write message size
                m_argStream.setPos(m_sizePosM);
                m_argStream.putInt32(curPos - m_sizePosM - 4);
                // restore stream pos
                m_argStream.setPos(curPos);
                // reset tag stream
                m_tagStream = WriteStream();
            }
        }

        //! Write integer message argument.
        /*!
         * Write a 32 bit integer message argument.
         *
         * \param arg 32 bit integer argument.
         *
         * \pre openMessage must have been called before with no intervening
         * closeMessage.
         *
         * \throw OSC::XRunError stream buffer xrun.
         */
        void putInt32(int32_t arg)
        {
            m_tagStream.putChar('i');
            m_argStream.putInt32(arg);
        }

        void putFloat32(float arg)
        {
            m_tagStream.putChar('f');
            m_argStream.putFloat32(arg);
        }

        void putString(const char* arg)
        {
            m_tagStream.putChar('s');
            m_argStream.putString(arg);
        }

        void putBlob(const blob_t& arg)
        {
            m_tagStream.putChar('b');
            m_argStream.putInt32(arg.size);
            m_argStream.putData(static_cast<const char*>(arg.data), arg.size);
        }

    private:
        char*       m_buffer;
        size_t      m_bufferSize;
        WriteStream	m_argStream;	// packet stream
        WriteStream	m_tagStream;	// current tag stream
        char*		m_sizePosM;     // last message size position
        char*       m_sizePosB;     // last bundle size position
        size_t		m_inBundle;		// bundle nesting depth
    };

    template <int Size> class StaticClientPacket : public ClientPacket
    {
    public:
        StaticClientPacket()
            : ClientPacket(m_buffer, Size)
        { }

    private:
        char m_buffer[Size];
    };
};

#endif // OSC_CLIENT_HH_INCLUDED

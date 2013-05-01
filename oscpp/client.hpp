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

#ifndef OSCPP_CLIENT_HPP_INCLUDED
#define OSCPP_CLIENT_HPP_INCLUDED

#include <oscpp/host.hpp>
#include <oscpp/stream.hpp>

#include <cstdint>
#include <limits>
#include <stdexcept>
#include <type_traits>

namespace OSC
{
namespace Client
{
    //! OSC packet construction.
    /*!
     * Construct a valid OSC packet for transmitting over a transport medium.
     */
    class Packet
    {
        int32_t calcSize(void* begin, void* end)
        {
            return static_cast<byte_t*>(end) - static_cast<byte_t*>(begin) - 4;
        }

    public:
        //! Constructor.
        /*!
        */
        Packet()
        {
            reset(0, 0);
        }

        //! Constructor.
        /*!
        */
        Packet(void* buffer, size_t size)
        {
            reset(buffer, size);
        }

        //! Get packet buffer address.
        /*!
         * Return the start address of the packet currently under construction.
         */
        void* data() const
        {
            return m_buffer;
        }

        size_t capacity() const
        {
            return m_capacity;
        }

        //! Get packet content size.
        /*!
         * Return the size of the packet currently under construction.
         */
        size_t size() const
        {
            return m_args.consumed();
        }

        //! Reset packet state.
        void reset(void* buffer, size_t size)
        {
            Stream::checkAlignment(&m_buffer, kMaxAlignment);
            m_buffer = buffer;
            m_capacity = size;
            m_args = WriteStream(m_buffer, m_capacity);
            m_sizePosM = m_sizePosB = 0;
            m_inBundle = 0;
        }

        void reset()
        {
            reset(m_buffer, m_capacity);
        }

        Packet& openBundle(uint64_t time)
        {
            if (m_inBundle) {
                // get current stream pos
                void* curPos = m_args.pos();
                // remember previous size pos offset
                *(int32_t*)curPos =
                      static_cast<byte_t*>(m_sizePosB)
                    - static_cast<byte_t*>(m_args.begin());
                m_args.skip(4);
                // record size pos
                m_sizePosB = curPos;
            }
            m_inBundle++;
            m_args.putString("#bundle");
            m_args.putUInt64(time);
            return *this;
        }

        Packet& closeBundle()
        {
            if (m_inBundle > 0) {
                if (m_inBundle > 1) {
                    // get current stream pos
                    byte_t* curPos = static_cast<byte_t*>(m_args.pos());
                    // get previous size pos
                    void* prevPos =
                          static_cast<byte_t*>(m_args.begin())
                        + *(int32_t*)m_sizePosB;
                    // write bundle size
                    m_args.setPos(m_sizePosB);
                    m_args.putInt32(calcSize(m_sizePosB, curPos));
                    // restore stream pos
                    m_args.setPos(curPos);
                    // record outer bundle size pos
                    m_sizePosB = prevPos;
                }
                m_inBundle--;
            } else {
                throw UnderrunError();
            }
            return *this;
        }

        Packet& openMessage(const char* addr, size_t numArgs)
        {
            if (m_inBundle) {
                // record message size pos
                m_sizePosM = m_args.pos();
                // advance arg stream
                m_args.skip(4);
            }
            m_args.putString(addr);
            size_t sigLen = numArgs+2;
            m_tags = WriteStream(m_args, sigLen);
            m_args.zero(WriteStream::align(sigLen));
            m_tags.putChar(',');
            return *this;
        }

        Packet& closeMessage()
        {
            if (m_inBundle) {
                // get current stream pos
                byte_t* curPos = static_cast<byte_t*>(m_args.pos());
                // write message size
                m_args.setPos(m_sizePosM);
                m_args.putInt32(calcSize(m_sizePosM, curPos));
                // restore stream pos
                m_args.setPos(curPos);
                // reset tag stream
                m_tags = WriteStream();
            }
            return *this;
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
        Packet& int32(int32_t arg)
        {
            m_tags.putChar('i');
            m_args.putInt32(arg);
            return *this;
        }

        Packet& float32(float arg)
        {
            m_tags.putChar('f');
            m_args.putFloat32(arg);
            return *this;
        }

        Packet& string(const char* arg)
        {
            m_tags.putChar('s');
            m_args.putString(arg);
            return *this;
        }

        // @throw std::invalid_argument if blob size is greater than std::numeric_limits<int32_t>::max()
        Packet& blob(const Blob& arg)
        {
            if (arg.size > std::numeric_limits<int32_t>::max())
                throw std::invalid_argument("Blob size greater than maximum value representable by int32_t");
            m_tags.putChar('b');
            m_args.putInt32(arg.size);
            m_args.putData(arg.data, arg.size);
            return *this;
        }

    private:
        void*       m_buffer;
        size_t      m_capacity;
        WriteStream m_args;         // packet stream
        WriteStream m_tags;         // current tag stream
        void*       m_sizePosM;     // last message size position
        void*       m_sizePosB;     // last bundle size position
        size_t      m_inBundle;     // bundle nesting depth
    };

    template <size_t buffer_size> class StaticPacket : public Packet
    {
    public:
        StaticPacket()
            : Packet(reinterpret_cast<byte_t*>(&m_buffer), buffer_size)
        { }

    private:
        typedef typename std::aligned_storage<buffer_size,kMaxAlignment>::type AlignedBuffer;
        AlignedBuffer m_buffer;
    };
}; // namespace Client
}; // namespace OSC

#endif // OSCPP_CLIENT_HPP_INCLUDED

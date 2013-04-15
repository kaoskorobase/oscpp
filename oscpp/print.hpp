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

#ifndef OSCPP_PRINT_HPP_INCLUDED
#define OSCPP_PRINT_HPP_INCLUDED

#include <oscpp/server.hpp>
#include <stdstream>

namespace OSC
{
    namespace detail
    {
        inline static void indent(std::ostream& out, size_t n)
        {
            while (n-- > 0) out << ' ';
        }
        inline static void printPacket(std::ostream& out, const OSC::Server::Bundle& bundle, size_t curIndent, size_t indentWidth)
        {
            for (auto p : bundle) {
                if (p.isMessage())
                    printMessage(p, curIndent);
                else
                    printBundle(p, curIndent, indentWidth);
            }
        }
    };

    std::ostream& operator<<(std::ostream& out, const OSC::Server::Message& msg)
    {
        out << msg.address() << ' ';
        OSC::Server::ArgStream args(msg.args());
        while (!args.atEnd()) {
            const char t = args.tag();
            out << t << ':';
            switch (t) {
                case 'i':
                    out << args.int32();
                    break;
                case 'f':
                    out << args.float32();
                    break;
                case 's':
                    out << args.string();
                    break;
                case 'b':
                    out << args.blob().size;
                    break;
                default:
                    out << '?';
                    break;
            }
            out << ' ';
        }
    }

    std::ostream& operator<<(std::ostream& out, const OSC::Server::Bundle& b)
    {
        for (auto p : b) {
            if (p.isMessage())
                out << (Message)p;
            else
                out << (Bundle)p;
        }
    }

    std::ostream& operator<<(std::ostream& out, const OSC::Server::Packet& packet)
    {
        if (packet.isMessage()) {
            out << packet;
        } else {
            OSC::Server::PacketStream ps(packet);
        }
        return out;
    }
};

#endif // OSCPP_PRINT_HPP_INCLUDED

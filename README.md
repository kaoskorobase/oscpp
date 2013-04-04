**oscpp** is a header-only C++11 library for constructing and parsing
[OpenSoundControl](http://opensoundcontrol.org) packets. Supported platforms
are MacOS X, Linux and Windows. **oscpp** intends to be a minimal,
high-performance solution for working with OSC data. The library doesn't
perform memory allocation (except when throwing exceptions) or other system
calls and is suitable for use in realtime sensitive contexts such as audio
driver callbacks.

**oscpp** conforms to the [OpenSoundControl 1.0
specification](http://opensoundcontrol.org/spec-1_0). Non-standard message
argument types are currently not supported and there is no direct support for
message address patterns or bundle scheduling; it is up to the user of the
library to implement (a subset of) the semantics according to the spec.

## Installation

Since **oscpp** only consists of header files, the library doesn't need to be
compiled or installed. Simply put the header file directory `oscpp` into
a location that is searched by your compiler and you're set.

## Usage

First let's have a look at how to construct OSC packets: Assuming you have
allocated a buffer before, you can construct a client packet on the stack and
start filling the buffer with data. When all the data has been written, the
`size()` method returns the actual size in bytes of the resulting OSC packet.

~~~~
#include <oscpp/client.hpp>

size_t makePacket(void* buffer, size_t size)
{
    // Construct a packet
    OSC::Client::Packet packet(buffer, size);
    packet
        // Open a bundle with a timetag
        .openBundle(1234UL)
            // Add a message with four arguments
            .openMessage("/s_new", 4)
                // Write the arguments
                .string("sinesweep")
                .int32(2)
                .string("freq")
                .float32(330.3)
            // Every `open` needs a corresponding `close`
            .closeMessage()
            // Add another message with one argument
            .openMessage("/n_free", 1)
                .int32(1)
            .closeMessage()
        .closeBundle();
    return packet.size();
}
~~~~

Now given some packet transport (e.g. a UDP socket), a packet can be
constructed and sent as follows:

~~~~
void sendPacket(Transport& t, void* buffer, size_t bufferSize)
{
    size_t packetSize = makePacket(buffer, bufferSize);
    t.write(buffer, packetSize);
}
~~~~

When parsing data from OSC packets you have to handle the two distinct cases of bundles and messages:

~~~~
#include <oscpp/server.hpp>
#include <iostream>

void handlePacket(const OSC::Server::Packet& packet)
{
    if (packet.isBundle()) {
        OSC::Server::Bundle bundle(packet);

        for (auto p : bundle) {
            // Just call this function recursively.
            // Might lead to stack overflow!
            handlePacket(p);
        }
    } else {
        OSC::Server::Message msg(packet);

        if (msg == "/s_new") {
            const char* name = args.string();
            const int32_t id = args.int32();
            const char* param = args.string();
            const float value = args.float32();
            std::cout << "/s_new" << ' '
                      << name << ' '
                      << id << ' '
                      << param << ' '
                      << value << std::endl;
        } else if (msg == "/n_set") {
            const int32_t id = args.int32();
            std::cout << "/n_set" << ' ' id << std::endl;
        } else {
            std::cout << "Unknown message: " << msg.address() << ' ';

            OSC::Server::ArgStream args(msg.args());
            while (!args.atEnd()) {
                const char tag = args.tag();
                std::cout << tag << ":(";
                switch (tag) {
                    case 'i': std::cout << args.int32(); break;
                    case 'f': std::cout << args.float32(); break;
                    case 's': std::cout << args.string(); break;
                    case 'b': std::cout << args.blob().size; << break;
                    default: args.drop();
                }
                std::cout << ") ";
            }

            std::cout << std::endl;
        }
    }
}
~~~~

Now we can receive data from a message based transport and pass it to our packet handling function:

~~~~
void recvPacket(Transport t&)
{
    void* buffer;
    size_t size;

    t.recv(&buffer, &size);

    try {
        handlePacket(OSC::Server::Packet(buffer, size));
    } catch (std::exception& e) {
        std::cout << "Exception: " << e.what() << std::endl;
    }

    t.free(buffer);
}
~~~~

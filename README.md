**oscpp** is a header-only C++11 library for constructing and parsing
[OpenSoundControl][OSC] packets. Supported platforms are MacOS X, Linux and
Windows. **oscpp** intend to be a minimal, high-performance solution for
working with OSC data. The library doesn't perform memory allocation (except
when throwing exceptions) or other system calls and is suitable for use in
realtime sensitive contexts such as audio driver callbacks.

## Installation

Since **oscpp** only consists of header files, the library doesn't need to be
compiled or installed. Simply put the directory containing the headers `oscpp`
into a location that is searched by your compiler and you're set.

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
void sendPacket(Transport t, void* buffer, size_t bufferSize)
{
    size_t packetSize = makePacket(buffer, bufferSize);
    t.write(buffer, packetSize);
}
~~~~

[OSC]: http://opensoundcontrol.org


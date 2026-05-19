[![CI](https://github.com/kaoskorobase/oscpp/actions/workflows/ci.yml/badge.svg)](https://github.com/kaoskorobase/oscpp/actions/workflows/ci.yml)

**oscpp** is a header-only C++11 library for constructing and parsing
[OpenSoundControl](http://opensoundcontrol.org) packets. It targets macOS,
iOS, Linux, Android and Windows, and is portable to any C++11 platform.
**oscpp** is a minimal, high-performance solution for working with OSC data:
it performs no memory allocation (except when throwing exceptions) and is
suitable for realtime-sensitive contexts such as audio driver callbacks.

**oscpp** conforms to the [OpenSoundControl 1.0
specification](http://opensoundcontrol.org/spec-1_0). Non-standard argument
types (except arrays) are not supported. Message address pattern matching and
bundle scheduling are left to the caller.

## Integration

**oscpp** is header-only. There are two ways to use it:

**Via CMake FetchContent** (recommended):

```cmake
include(FetchContent)
FetchContent_Declare(oscpp
    GIT_REPOSITORY https://github.com/kaoskorobase/oscpp.git
    GIT_TAG        1.0.0
)
FetchContent_MakeAvailable(oscpp)

target_link_libraries(myapp PRIVATE oscpp::oscpp)
```

**Include directory only** — put the `include` directory into a location that
is searched by your compiler and you're set. No compilation or installation
required.

## Building and Testing

```shell
cmake --preset debug                               # or: --preset release
cmake --build build/debug                          # or: build/release
ctest --test-dir build/debug --output-on-failure   # or: build/release
```

## Usage

**oscpp** places everything in the `OSCPP` namespace, with the two most
important subnamespaces `Client` for constructing packets and `Server` for
parsing packets.

To build an OSC packet, construct a `Client::Packet` over a buffer and chain
method calls to write the data. The `size` method returns the final packet
size in bytes.

```cpp
#include <oscpp/client.hpp>

size_t makePacket(void* buffer, size_t size)
{
    // Construct a packet
    OSCPP::Client::Packet packet(buffer, size);
    packet
        // Open a bundle with a timetag
        .openBundle(1234ULL)
            // Add a message with two arguments and an array with 6 elements;
            // for efficiency this needs to be known in advance.
            .openMessage("/s_new", 2 + OSCPP::Tags::array(6))
                // Write the arguments
                .string("sinesweep")
                .int32(2)
                .openArray()
                    .string("start-freq")
                    .float32(330.0f)
                    .string("end-freq")
                    .float32(990.0f)
                    .string("amp")
                    .float32(0.4f)
                .closeArray()
            // Every `open` needs a corresponding `close`
            .closeMessage()
            // Add another message with one argument
            .openMessage("/n_free", 1)
                .int32(1)
            .closeMessage()
            // And nother one
            .openMessage("/n_set", 3)
                .int32(1)
                .string("wobble")
                // Numeric arguments are converted automatically
                // (see below)
                .int32(31)
            .closeMessage()
        .closeBundle();
    return packet.size();
}
```

Given a transport (e.g. a UDP socket or in-memory FIFO; see the appendix for a
minimal implementation), sending a packet looks like this:

```cpp
class Transport;

size_t send(Transport* t, const void* buffer, size_t size);

void sendPacket(Transport* t, void* buffer, size_t bufferSize)
{
    const size_t packetSize = makePacket(buffer, bufferSize);
    send(t, buffer, packetSize);
}
```

Parsing requires handling two cases — bundles and messages:

```cpp
#include <oscpp/server.hpp>
#include <oscpp/print.hpp>
#include <iostream>

void handlePacket(const OSCPP::Server::Packet& packet)
{
    if (packet.isBundle()) {
        // Convert to bundle
        OSCPP::Server::Bundle bundle(packet);

        // Print the time
        std::cout << "#bundle " << bundle.time() << std::endl;

        // Get packet stream
        OSCPP::Server::PacketStream packets(bundle.packets());

        // Iterate over all the packets and call handlePacket recursively.
        // Cuidado: Might lead to stack overflow!
        while (!packets.atEnd()) {
            handlePacket(packets.next());
        }
    } else {
        // Convert to message
        OSCPP::Server::Message msg(packet);

        // Get argument stream
        OSCPP::Server::ArgStream args(msg.args());

        // Directly compare message address to string with operator==.
        // For handling larger address spaces you could use e.g. a
        // dispatch table based on std::unordered_map.
        if (msg == "/s_new") {
            const char* name = args.string();
            const int32_t id = args.int32();
            std::cout << "/s_new" << " "
                      << name << " "
                      << id << " ";
            // Get the params array as an ArgStream
            OSCPP::Server::ArgStream params(args.array());
            while (!params.atEnd()) {
                const char* param = params.string();
                const float value = params.float32();
                std::cout << param << ":" << value << " ";
            }
            std::cout << std::endl;
        } else if (msg == "/n_set") {
            const int32_t id = args.int32();
            const char* key = args.string();
            // Numeric arguments are converted automatically
            // to float32 (e.g. from int32).
            const float value = args.float32();
            std::cout << "/n_set" << " "
                      << id << " "
                      << key << " "
                      << value << std::endl;
        } else {
            // Simply print unknown messages
            std::cout << "Unknown message: " << msg << std::endl;
        }
    }
}
```

Receiving from a message-based transport:

```cpp
#include <array>

const size_t kMaxPacketSize = 8192;

size_t recv(Transport* t, void* buffer, size_t size);

void recvPacket(Transport* t)
{
    std::array<char,kMaxPacketSize> buffer;
    size_t size = recv(t, buffer.data(), buffer.size());
    handlePacket(OSCPP::Server::Packet(buffer.data(), size));
}
```

Putting it together:

```cpp
#include <memory>
#include <stdexcept>

Transport* newTransport();

int main(int, char**)
{
    std::unique_ptr<Transport> t(newTransport());
    std::array<char,kMaxPacketSize> sendBuffer;
    try {
        sendPacket(t.get(), sendBuffer.data(), sendBuffer.size());
        recvPacket(t.get());
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    return 0;
}
```

Compiling and running the example produces the following output:

```
#bundle 1234
/s_new sinesweep 2 start-freq:330 end-freq:990 amp:0.4
Unknown message: /n_free i:1
/n_set 1 wobble 31
```

## Appendix: Support code

Here's the code for a trivial transport that has a single packet buffer:

```cpp
#include <cstring>

class Transport
{
public:
    size_t send(const void* buffer, size_t size)
    {
        size_t n = std::min(m_buffer.size(), size);
        std::memcpy(m_buffer.data(), buffer, n);
        m_message = n;
        return n;
    }

    size_t recv(void* buffer, size_t size)
    {
        if (m_message > 0) {
            size_t n = std::min(m_message, size);
            std::memcpy(buffer, m_buffer.data(), n);
            m_message = 0;
            return n;
        }
        return 0;
    }

private:
    std::array<char,kMaxPacketSize> m_buffer;
    size_t m_message;
};

Transport* newTransport()
{
    return new Transport;
}

size_t send(Transport* t, const void* buffer, size_t size)
{
    return t->send(buffer, size);
}

size_t recv(Transport* t, void* buffer, size_t size)
{
    return t->recv(buffer, size);
}
```

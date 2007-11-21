// vim: et sw=4:
#include <iostream>
#include <machine/endian.h>
#include <netinet/in.h>

#define BUFFER_SIZE 256
char buffer[BUFFER_SIZE];

#define OSC_HOST_LITTLE_ENDIAN 0
#define OSC_MAX_BUNDLE_NESTING 1

#include "OSC_Client.hh"
#include "OSC_Server.hh"

static void parseMessage(uint64_t t, const char* address, OSC::ArgStream& args, void* data)
{
    std::cout << "parseMessage\n";

    std::cout
	<< "[" << t << "] "
	<< address << ' ';

    while (!args.atEnd())
    {
	switch (args.peekTag())
	{
	    case 'i': std::cout << args.getInt32(); break;
	    case 'f': std::cout << args.getFloat32(); break;
	    case 's': std::cout << args.getString(); break;
	    default: std::cout << "<unknown>";
	};

	std::cout << ' ';
    }
    std::cout << '\n';
}

int main()
{
    size_t packetSize = 0;

    try
    {
	OSC::ClientPacket cp(buffer, BUFFER_SIZE);
	cp.openBundle(1);
	// write msg 1
	cp.openMessage("/foo");
	cp.setNumArgs(3);
	cp.putFloat32(12.1232221);
	cp.putString("bar");
	cp.putInt32(13);
	cp.closeMessage();
	// write msg 2
	cp.openMessage("/gee");
	cp.setNumArgs(5);
	cp.putString("12.1232221");
	cp.putString("hahahaha");
	cp.putInt32(144);
	cp.putString("jhsgdi..asjhg...ahsgdh");
	cp.putFloat32(23.4);
	cp.closeMessage();
	cp.closeBundle();
	packetSize = cp.getSize();
    }
    catch (OSC::Error& e)
    {
	std::cerr << e.what() << '\n';
	return 1;
    }

    std::cout << "packet size: " << packetSize << '\n';
    FILE* f = fopen("packet.bin", "w");
    fwrite(buffer, 1, packetSize, f);
    fclose(f);
    f = fopen("packet.bin", "r");
    fread(buffer, 1, packetSize, f);
    fclose(f);

    try
    {
	OSC::ServerPacket sp(buffer, packetSize);
        OSC::MessageStream ms(sp);
        while (!ms.atEnd()) {
            OSC::Message m = ms.next();
            OSC::ArgStream args(m.data, m.size);
            parseMessage(sp.time, m.path, args, 0);
        }
    }
    catch (OSC::Error& e)
    {
	std::cerr << e.what() << '\n';
	return 1;
    }

    return 0;
}

// EOF

#include <oscpp/client.hpp>
#include <oscpp/print.hpp>
#include <oscpp/server.hpp>

#include <catch2/catch_all.hpp>
#include <rapidcheck.h>
#include <rapidcheck/catch.h>

#include <cstdint>
#include <list>
#include <memory>
#include <string>

namespace OSCPP { namespace AST {
class Value
{
public:
    virtual ~Value()
    {}
    virtual void print(std::ostream& out) const = 0;
    virtual void put(OSCPP::Client::Packet& packet) const = 0;
};

template <class T> using List = std::list<std::shared_ptr<T>>;

template <class T> bool equalList(const List<T>& list1, const List<T>& list2)
{
    if (list1.size() != list2.size())
        return false;
    auto it1 = list1.begin();
    auto it2 = list2.begin();
    while ((it1 != list1.end()) && (it2 != list2.end()))
    {
        if (**it1 == **it2)
        {
            it1++;
            it2++;
        }
        else
        {
            return false;
        }
    }
    return true;
}

template <class T> void printList(std::ostream& out, const List<T>& list)
{
    const size_t n = list.size();
    size_t       i = 1;
    out << '[';
    for (auto x : list)
    {
        x->print(out);
        if (i != n)
        {
            out << ',';
        }
        i++;
    }
    out << ']';
}

class Argument : public Value
{
public:
    enum Type
    {
        kInt32,
        kFloat32,
        kString,
        kBlob,
        kArray,
    };
    static constexpr size_t kNumTypes = kArray + 1;

    Argument(Type type)
    : m_type(type)
    {}

    Type type() const
    {
        return m_type;
    }

    virtual size_t numTags() const
    {
        return 1;
    }

    static size_t numTags(const List<Argument>& args)
    {
        size_t n = 0;
        for (auto x : args)
            n += x->numTags();
        return n;
    }

    bool operator==(const Argument& other)
    {
        return (type() == other.type()) && equals(other);
    }

    virtual size_t size() const = 0;

protected:
    virtual bool equals(const Argument& other) const = 0;

private:
    Type m_type;
};

class Bundle;
class Message;

class Packet : public Value
{
public:
    enum Type
    {
        kMessage,
        kBundle
    };

    Packet(Type type)
    : m_type(type)
    {}

    Type type() const
    {
        return m_type;
    }

    virtual size_t size() const = 0;

    static std::shared_ptr<Packet> parse(const OSCPP::Server::Packet& packet)
    {
        return packet.isBundle() ? parseBundle(packet) : parseMessage(packet);
    }

    bool operator==(const Packet& other) const
    {
        return type() == other.type() && equals(other);
    }

protected:
    virtual bool equals(const Packet& other) const = 0;

private:
    static std::shared_ptr<Packet>
                parseBundle(const OSCPP::Server::Bundle& bdl);
    static void parseArgs(OSCPP::Server::ArgStream& inArgs,
                          List<Argument>&           outArgs);
    static std::shared_ptr<Packet>
    parseMessage(const OSCPP::Server::Message& msg);

    Type m_type;
};

class Bundle : public Packet
{
public:
    Bundle(uint64_t time, List<Packet> packets)
    : Packet(kBundle)
    , m_time(time)
    , m_packets(packets)
    {
        // assert(packets.size() > 0);
    }

    void print(std::ostream& out) const override
    {
        out << "Bundle(" << m_time << ", ";
        printList(out, m_packets);
        out << ')';
    }

    void put(OSCPP::Client::Packet& packet) const override
    {
        packet.openBundle(m_time);
        for (auto p : m_packets)
            p->put(packet);
        packet.closeBundle();
    }

    size_t size() const override
    {
        size_t payload = 0;
        for (auto x : m_packets)
            payload += x->size();
        assert(OSCPP::isAligned(payload));
        return OSCPP::Size::bundle(m_packets.size()) + payload;
    }

protected:
    bool equals(const Packet& other) const override
    {
        const auto& otherBundle = dynamic_cast<const Bundle&>(other);
        return m_time == otherBundle.m_time &&
               equalList(m_packets, otherBundle.m_packets);
    }

private:
    uint64_t     m_time;
    List<Packet> m_packets;
};

class Message : public Packet
{
public:
    Message(std::string address, List<Argument> args)
    : Packet(kMessage)
    , m_address(address)
    , m_args(args)
    {}

    void print(std::ostream& out) const override
    {
        out << "Message(" << m_address << ", ";
        printList(out, m_args);
        out << ')';
    }

    void put(OSCPP::Client::Packet& packet) const override
    {
        packet.openMessage(m_address.c_str(), Argument::numTags(m_args));
        for (auto x : m_args)
            x->put(packet);
        packet.closeMessage();
    }

    size_t size() const override
    {
        size_t payload = 0;
        for (auto x : m_args)
            payload += x->size();
        assert(OSCPP::isAligned(payload));
        return OSCPP::Size::message(OSCPP::Size::String(m_address.c_str()),
                                    Argument::numTags(m_args)) +
               payload;
    }

protected:
    bool equals(const Packet& other) const override
    {
        const auto& otherMsg = dynamic_cast<const Message&>(other);
        return m_address == otherMsg.m_address &&
               equalList(m_args, otherMsg.m_args);
    }

private:
    std::string    m_address;
    List<Argument> m_args;
};

class Int32 : public Argument
{
public:
    Int32(int32_t value)
    : Argument(kInt32)
    , m_value(value)
    {}

    void print(std::ostream& out) const override
    {
        out << "i:" << m_value;
    }

    void put(OSCPP::Client::Packet& packet) const override
    {
        packet.put(m_value);
    }

    size_t size() const override
    {
        return OSCPP::Size::int32();
    }

protected:
    bool equals(const Argument& other) const override
    {
        return dynamic_cast<const Int32&>(other).m_value == m_value;
    }

private:
    int32_t m_value;
};

class Float32 : public Argument
{
public:
    Float32(float value)
    : Argument(kFloat32)
    , m_value(value)
    {}

    void print(std::ostream& out) const override
    {
        out << "f:" << m_value;
    }

    void put(OSCPP::Client::Packet& packet) const override
    {
        packet.put(m_value);
    }

    size_t size() const override
    {
        return OSCPP::Size::float32();
    }

protected:
    bool equals(const Argument& other) const override
    {
        return dynamic_cast<const Float32&>(other).m_value == m_value;
    }

private:
    float m_value;
};

class String : public Argument
{
public:
    String(std::string value)
    : Argument(kString)
    , m_value(value)
    {}

    void print(std::ostream& out) const override
    {
        out << "s:" << m_value;
    }

    void put(OSCPP::Client::Packet& packet) const override
    {
        packet.put(m_value.c_str());
    }

    size_t size() const override
    {
        return OSCPP::Size::string(OSCPP::Size::String(m_value.c_str()));
    }

protected:
    bool equals(const Argument& other) const override
    {
        return dynamic_cast<const String&>(other).m_value == m_value;
    }

private:
    std::string m_value;
};

class Blob : public Argument
{
public:
    Blob(int32_t size, const void* data = nullptr)
    : Argument(kBlob)
    , m_size(std::max(0, size))
    , m_data(nullptr)
    {
        if (m_size > 0)
        {
            m_data = new char[m_size];
            if (data != nullptr)
                std::memcpy(m_data, data, m_size);
        }
    }

    Blob(OSCPP::Blob b)
    : Blob(static_cast<int32_t>(b.size()), b.data())
    {}

    ~Blob()
    {
        delete[] m_data;
    }

    void print(std::ostream& out) const override
    {
        out << "b:" << m_size;
    }

    void put(OSCPP::Client::Packet& packet) const override
    {
        packet.put(OSCPP::Blob(m_data, m_size));
    }

    size_t size() const override
    {
        return OSCPP::Size::blob(m_size);
    }

protected:
    bool equals(const Argument& other) const override
    {
        const Blob& otherBlob = dynamic_cast<const Blob&>(other);
        return otherBlob.m_size == m_size &&
               memcmp(m_data, otherBlob.m_data, m_size) == 0;
    }

private:
    size_t m_size;
    char*  m_data;
};

class Array : public Argument
{
public:
    Array(List<Argument> elems = List<Argument>())
    : Argument(kArray)
    , m_elems(elems)
    {}

    void print(std::ostream& out) const override
    {
        printList(out, m_elems);
    }

    void put(OSCPP::Client::Packet& packet) const override
    {
        packet.openArray();
        for (auto x : m_elems)
            x->put(packet);
        packet.closeArray();
    }

    size_t size() const override
    {
        size_t payload = 0;
        for (auto x : m_elems)
            payload += x->size();
        assert(OSCPP::isAligned(payload));
        return payload;
    }

    size_t numTags() const override
    {
        return OSCPP::Tags::array(Argument::numTags(m_elems));
    }

protected:
    bool equals(const Argument& other) const override
    {
        return equalList(m_elems, dynamic_cast<const Array&>(other).m_elems);
    }

private:
    List<Argument> m_elems;
};

std::shared_ptr<Packet> Packet::parseBundle(const OSCPP::Server::Bundle& bdl)
{
    List<Packet>                outPackets;
    OSCPP::Server::PacketStream inPackets(bdl.packets());
    while (!inPackets.atEnd())
    {
        outPackets.push_back(parse(inPackets.next()));
    }
    return std::make_shared<Bundle>(bdl.time(), std::move(outPackets));
}

void Packet::parseArgs(OSCPP::Server::ArgStream& inArgs,
                       List<Argument>&           outArgs)
{
    while (!inArgs.atEnd())
    {
        switch (inArgs.tag())
        {
            case 'i':
                outArgs.push_back(std::make_shared<Int32>(inArgs.int32()));
                break;
            case 'f':
                outArgs.push_back(std::make_shared<Float32>(inArgs.float32()));
                break;
            case 's':
                outArgs.push_back(std::make_shared<String>(inArgs.string()));
                break;
            case 'b':
                outArgs.push_back(std::make_shared<Blob>(inArgs.blob()));
                break;
            case '[':
            {
                OSCPP::Server::ArgStream inElems(inArgs.array());
                List<Argument>           outElems;
                parseArgs(inElems, outElems);
                outArgs.push_back(std::make_shared<Array>(outElems));
            }
            break;
        }
    }
}

std::shared_ptr<Packet> Packet::parseMessage(const OSCPP::Server::Message& msg)
{
    OSCPP::Server::ArgStream inArgs(msg.args());
    List<Argument>           outArgs;
    parseArgs(inArgs, outArgs);
    return std::make_shared<Message>(msg.address(), outArgs);
}

std::ostream& operator<<(std::ostream& out, const Packet& packet)
{
    packet.print(out);
    return out;
}

std::ostream& operator<<(std::ostream&                  out,
                         const std::shared_ptr<Packet>& packet)
{
    packet->print(out);
    return out;
}
}} // namespace OSCPP::AST

// Forward declaration for mutual recursion between genArgList and Arbitrary<Argument>
namespace {
    rc::Gen<OSCPP::AST::List<OSCPP::AST::Argument>> genArgList();
}

namespace rc {

template <>
struct Arbitrary<std::shared_ptr<OSCPP::AST::Argument>> {
    static Gen<std::shared_ptr<OSCPP::AST::Argument>> arbitrary()
    {
        using A = OSCPP::AST::Argument;
        return gen::lazy([] {
            return gen::mapcat(
                gen::inRange(0, static_cast<int>(A::kNumTypes)),
                [](int t) -> Gen<std::shared_ptr<A>> {
                    switch (static_cast<A::Type>(t)) {
                        case A::kInt32:
                            return gen::map(gen::arbitrary<int32_t>(),
                                [](int32_t v) -> std::shared_ptr<A> {
                                    return std::make_shared<OSCPP::AST::Int32>(v); });
                        case A::kFloat32:
                            return gen::map(gen::arbitrary<float>(),
                                [](float v) -> std::shared_ptr<A> {
                                    return std::make_shared<OSCPP::AST::Float32>(v); });
                        case A::kString:
                            return gen::map(
                                gen::container<std::string>(gen::inRange<char>(33, 127)),
                                [](std::string s) -> std::shared_ptr<A> {
                                    return std::make_shared<OSCPP::AST::String>(s); });
                        case A::kBlob:
                            return gen::map(gen::inRange<int32_t>(0, 64),
                                [](int32_t sz) -> std::shared_ptr<A> {
                                    return std::make_shared<OSCPP::AST::Blob>(sz); });
                        case A::kArray:
                            return gen::map(gen::scale(0.5, genArgList()),
                                [](OSCPP::AST::List<OSCPP::AST::Argument> elems) -> std::shared_ptr<A> {
                                    return std::make_shared<OSCPP::AST::Array>(std::move(elems)); });
                        default:
                            return gen::just(std::shared_ptr<A>(
                                std::make_shared<OSCPP::AST::Int32>(0)));
                    }
                });
        });
    }
};

template <>
struct Arbitrary<std::shared_ptr<OSCPP::AST::Packet>> {
    static Gen<std::shared_ptr<OSCPP::AST::Packet>> arbitrary()
    {
        using P = OSCPP::AST::Packet;
        return gen::lazy([] {
            return gen::mapcat(
                gen::arbitrary<bool>(),
                [](bool isBundle) -> Gen<std::shared_ptr<P>> {
                    if (isBundle) {
                        return gen::map(
                            gen::tuple(
                                gen::arbitrary<uint64_t>(),
                                gen::scale(0.5,
                                    gen::container<std::vector<std::shared_ptr<P>>>(
                                        gen::arbitrary<std::shared_ptr<P>>()))),
                            [](std::tuple<uint64_t, std::vector<std::shared_ptr<P>>> t)
                                    -> std::shared_ptr<P> {
                                uint64_t time = std::get<0>(t);
                                auto& pkts = std::get<1>(t);
                                OSCPP::AST::List<P> list(pkts.begin(), pkts.end());
                                return std::make_shared<OSCPP::AST::Bundle>(time, list);
                            });
                    } else {
                        auto addrGen = gen::map(
                            gen::container<std::string>(gen::inRange<char>('a', '{')),
                            [](std::string s) -> std::string { return "/" + s; });
                        return gen::map(
                            gen::tuple(addrGen, genArgList()),
                            [](std::tuple<std::string,
                                         OSCPP::AST::List<OSCPP::AST::Argument>> t)
                                    -> std::shared_ptr<P> {
                                std::string addr = std::get<0>(t);
                                OSCPP::AST::List<OSCPP::AST::Argument> args =
                                    std::get<1>(t);
                                return std::make_shared<OSCPP::AST::Message>(addr, args);
                            });
                    }
                });
        });
    }
};

} // namespace rc

namespace {
    rc::Gen<OSCPP::AST::List<OSCPP::AST::Argument>> genArgList()
    {
        using A = OSCPP::AST::Argument;
        return rc::gen::map(
            rc::gen::container<std::vector<std::shared_ptr<A>>>(
                rc::gen::arbitrary<std::shared_ptr<A>>()),
            [](std::vector<std::shared_ptr<A>> v) {
                return OSCPP::AST::List<A>(v.begin(), v.end());
            });
    }
}

TEST_CASE("prop_identity")
{
    rc::prop("identity round-trips encode/decode",
        [](std::shared_ptr<OSCPP::AST::Packet> p) {
            const size_t            size = p->size();
            std::unique_ptr<char[]> data(new char[size]);
            OSCPP::Client::Packet   clientPacket(data.get(), size);
            p->put(clientPacket);
            OSCPP::Server::Packet serverPacket(clientPacket.data(),
                                               clientPacket.size());
            auto p2 = OSCPP::AST::Packet::parse(serverPacket);
            RC_ASSERT(*p == *p2);
        });
}

TEST_CASE("prop_overflow")
{
    rc::prop("overflow throws OverflowError for undersized buffer",
        [](std::shared_ptr<OSCPP::AST::Packet> packet, size_t inBufferSize) {
            const size_t packetSize = packet->size();
            const size_t bufferSize =
                inBufferSize == 0
                    ? 1
                    : (inBufferSize < packetSize ? inBufferSize : packetSize - 1);
            std::unique_ptr<char[]> data(new char[bufferSize]);
            OSCPP::Client::Packet   clientPacket(data.get(), bufferSize);
            bool                    threw = false;
            try
            {
                packet->put(clientPacket);
            }
            catch (OSCPP::OverflowError&)
            {
                threw = true;
            }
            catch (OSCPP::UnderrunError&)
            {
                // openMessage throws UnderrunError when the tag sub-stream
                // would exceed the buffer bounds
                threw = true;
            }
            RC_ASSERT(threw);
        });
}

TEST_CASE("tag_string_overflow_message")
{
    // Buffer large enough for address but not for tag string
    char   buf[8];
    OSCPP::Client::Packet p(buf, sizeof(buf));
    try
    {
        p.openMessage("/x", 10); // 10 tags won't fit in 8 bytes
        FAIL("Expected OverflowError");
    }
    catch (OSCPP::OverflowError& e)
    {
        REQUIRE(std::string(e.what()).find("Tag string overflow") != std::string::npos);
    }
}

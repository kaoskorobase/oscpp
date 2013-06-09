#include <autocheck/autocheck.hpp>
#include <oscpp/client.hpp>
#include <cstdint>
#include <list>
#include <memory>
#include <string>

namespace oscpp { namespace ast {
    class Value
    {
    public:
        virtual void print(std::ostream& out) const = 0;
        virtual void put(OSC::Client::Packet& packet) const = 0;
        // virtual void get(OSC::Server::Packet& packet) = 0;
    };

    template <class T> using List = std::list<std::shared_ptr<T>>;

    template <class T> void printList(std::ostream& out, const List<T>& list)
    {
        const size_t n = list.size();
        size_t i = 1;
        out << '[';
        for (auto x : list) {
            x->print(out);
            if (i != n) {
                out << ',';
            }
            i++;
        }
        out << ']';
    }
    class Packet : public Value
    {
    };
    class Bundle : public Packet
    {
    public:
        Bundle(uint64_t time, List<Packet> packets)
            : m_time(time)
            , m_packets(packets)
        { }
        void print(std::ostream& out) const override
        {
            out << "Bundle("
                << m_time
                << ", ";
            printList(out, m_packets);
            out << ')';
        }
        void put(OSC::Client::Packet& packet) const override
        {
            packet.openBundle(m_time);
            for (auto p : m_packets) p->put(packet);
            packet.closeBundle();
        }
    private:
        uint64_t m_time;
        List<Packet> m_packets;
    };
    class Argument : public Value
    {
    public:
        enum Type
        {
            kInt32,
            kFloat32,
            kString,
            kArray,
        };
        static constexpr size_t kNumTypes = kArray + 1;

        virtual size_t numArgs() const
        {
            return 1;
        }
        static size_t numArgs(const List<Argument>& args)
        {
            size_t n = 0;
            for (auto x : args) n += x->numArgs();
            return n;
        }
    };
    class Message : public Packet
    {
    public:
        Message(std::string address, List<Argument> args)
            : m_address(address)
            , m_args(args)
        { }
        void print(std::ostream& out) const override
        {
            out << "Message("
                << m_address
                << ", ";
            printList(out, m_args);
            out << ')';
        }
        void put(OSC::Client::Packet& packet) const override
        {
            packet.openMessage(m_address.c_str(), Argument::numArgs(m_args));
            for (auto x : m_args) x->put(packet);
            packet.closeMessage();
        }
    private:
        std::string m_address;
        List<Argument> m_args;
    };
    class Int32 : public Argument
    {
    public:
        Int32(int32_t value)
            : m_value(value)
        { }
        void print(std::ostream& out) const override
        {
            out << "i:" << m_value;
        }
        void put(OSC::Client::Packet& packet) const override
        {
            packet.put(m_value);
        }
    private:
        int32_t m_value;
    };
    class Float32 : public Argument
    {
    public:
        Float32(float value)
            : m_value(value)
        { }
        void print(std::ostream& out) const override
        {
            out << "f:" << m_value;
        }
        void put(OSC::Client::Packet& packet) const override
        {
            packet.put(m_value);
        }
    private:
        float m_value;
    };
    class String : public Argument
    {
    public:
        String(std::string value)
            : m_value(value)
        { }
        void print(std::ostream& out) const override
        {
            out << "s:" << m_value;
        }
        void put(OSC::Client::Packet& packet) const override
        {
            packet.put(m_value.c_str());
        }
    private:
        std::string m_value;
    };
    class Array : public Argument
    {
    public:
        Array(List<Argument> elems)
            : m_elems(elems)
        { }
        void print(std::ostream& out) const override
        {
            printList(out, m_elems);
        }
        void put(OSC::Client::Packet& packet) const override
        {
            packet.openArray();
            for (auto x : m_elems) x->put(packet);
            packet.closeArray();
        }
        size_t numArgs() const override
        {
            return m_elems.size() + 2;
        }
    private:
        List<Argument> m_elems;
    };
} }

namespace ac = autocheck;

namespace oscpp { namespace autocheck {
struct argument_gen
{
    typedef std::shared_ptr<ast::Argument> result_type;
    result_type operator() (size_t size)
    {
        ast::Argument::Type argType = static_cast<ast::Argument::Type>(ac::generator<unsigned int>()(ast::Argument::kNumTypes));
        switch (argType) {
            case ast::Argument::kInt32:
                return std::make_shared<ast::Int32>(ac::generator<int32_t>()(size));
            case ast::Argument::kFloat32:
                return std::make_shared<ast::Float32>(ac::generator<float>()(size));
            case ast::Argument::kString:
                return std::make_shared<ast::String>(ac::generator<std::string>()(std::max(1ul, size)));
            case ast::Argument::kArray:
                // return std::make_shared<ast::Array>(ac::list_of<argumen_gen>()(size));
                return std::make_shared<ast::Array>(ast::List<ast::Argument>());
        }
        return nullptr;
    }
};
struct packet_gen
{
    // ac::generator<std::shared_ptr<oscpp::ast::Packet>> source;
    typedef std::shared_ptr<ast::Packet> result_type;
    result_type operator() (size_t size) {
        return ac::generator<bool>()(size)
            ? gen_bundle(size)
            : gen_message(size);
    }

    result_type gen_bundle(size_t size)
    {
        return std::make_shared<ast::Bundle>(ac::generator<uint64_t>()(size), ast::List<ast::Packet>());
    }
    std::string gen_message_address(size_t size)
    {
        std::string result(ac::generator<std::string>()(std::max<size_t>(2, size)));
        if (result[0] != '/') result[0] = '/';
        return std::move(result);
    }
    result_type gen_message(size_t size)
    {
        return std::make_shared<ast::Message>(gen_message_address(size), ast::List<ast::Argument>());
    }
};
} }

// template <typename T>
// struct ordered_list_gen {
//   ac::generator<std::vector<T>> source;
// 
//   typedef std::vector<T> result_type;
// 
//   std::vector<T> operator() (size_t size) {
//     result_type xs(source(size));
//     std::sort(xs.begin(), xs.end());
//     return std::move(xs);
//   }
// };
// 
int main(int argc, char** argv)
{
    using namespace oscpp::autocheck;
    using namespace oscpp::ast;
    auto packet = packet_gen()(100);
    packet->print(std::cout);
    std::cout << std::endl;
    return 0;
}

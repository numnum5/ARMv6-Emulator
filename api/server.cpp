#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio.hpp>

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#include <glaze/glaze.hpp>

// #include "cpu.hpp"
#include "emulator.hpp"
// #include "compiler.hpp"


struct CpuState {
    uint32_t regs[16];   // R0-R14
    uint32_t pc;         // R15 / PC
    uint64_t cycles;
    bool halted;
};

template <>
struct glz::meta<CpuState> {
    using T = CpuState;

    static constexpr auto value = glz::object(
        "regs",    &T::regs,
        "pc",      &T::pc,
        "cycles",  &T::cycles,
        "halted",  &T::halted
    );
};

struct WsResponse {
    std::string type;           // "state", "error", "ok"
    std::optional<CpuState> state;
    std::optional<std::string> message;
};

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;

int main()
{
    try
    {
        net::io_context io_context;

        tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 8080));

        std::cout << "Waiting for connection on ws://localhost:8080\n";

        tcp::socket socket(io_context);

        acceptor.accept(socket);

        websocket::stream<tcp::socket> ws(std::move(socket));

        ws.accept();

        std::cout << "Client connected\n";

        int counter = 0;

        beast::flat_buffer buffer;

        // intialise 
        
        Emulator emu;
        
        emu.load_elf("firmware.elf");
        emu.startCpu();


        while (true)
        {
            ws.read(buffer);
            // glz::write().value_or("error");
            std::string message = beast::buffers_to_string(buffer.data());

            buffer.consume(buffer.size());
            
            std::cout
                << "Received: "
                << message
                << '\n';

            if (message == "step") {
                emu.cpu.step();

                CpuState state{
                    // .regs = {0},
                    .pc = emu.cpu.fetch_pc,
                    .cycles = emu.cpu.cycle,
                    .halted = false
                };
                std::memcpy(state.regs, emu.cpu.regs, sizeof(state.regs));

                std::string json;
                glz::write_json(state, json);

                ws.text(true);
                ws.write(boost::asio::buffer(json));
                // ws.text(true);
                // ws.write(net::buffer(glz::write_json(resp).value()));
                // ws.write(net::buffer("cycle executed"));
            }
            else if (message == "cycle")
            {
                  ws.write(net::buffer(
                    "cycle executed"));
            }
            else
            {
                ws.write(net::buffer(
                    "unknown command"));
            }
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << '\n';
    }

    return 0;
}
#include <iostream>
#include "emulator.hpp"
#include "compiler.hpp"

int main(int argc, char ** argv)
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0]
                  << " <elf file>\n";
        return 1;
    }

    std::string filename = argv[1];

    int status = compile(filename);

    if (status != 0)
    {
        return 1;
    }

    Emulator emu;
    emu.load_elf("firmware.elf");
    emu.startCpu();
}
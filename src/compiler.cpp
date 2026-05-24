#include "compiler.hpp"

int compile(const std::string& filename)
{
    const std::string command =
        std::string("arm-none-eabi-gcc ")
        + "-mcpu=cortex-m0 "
        + "-mthumb "
        + "-ffreestanding "
        + "-nostdlib "
        + "-T ./elf/linker.ld "
        + "-I./elf/include "
        + "./elf/src/setup.c "
        + "./elf/src/semihosting.c "
        + "./elf/src/"
        + filename
        + " -o firmware.elf";
        
    return std::system(command.c_str());
}
#include "compiler.hpp"

int compile(const std::string& filename)
{
    std::string command =
        std::string("arm-none-eabi-gcc ")
        + "-mcpu=cortex-m0 "
        + "-mthumb "
        + "-ffreestanding "
        + "-nostdlib "
        + "-T ./elf/linker.ld "
        + "./elf/setup.c "
        + "./elf/"
        + filename
        + " -o firmware.elf";
        
    return std::system(command.c_str());
}
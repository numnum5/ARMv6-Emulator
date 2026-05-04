#include "compiler.hpp"

int compile(std::string& filename) 
{
    int status = std::system(
        "arm-none-eabi-gcc "
        "-mcpu=cortex-m0 "
        "-mthumb "
        "-ffreestanding "
        "-nostdlib "
        "../src/elf.c "
        "-o main.elf"
    );

    if (status == 0) {
        std::cout << "Compilation finished\n";
    } else {
        std::cerr << "Compilation failed with code: " << status << "\n";
    }

    return status;
}
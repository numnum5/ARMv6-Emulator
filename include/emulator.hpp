#include <iostream>
#include <vector>
#include <limits>
#include <bitset>
#include <bit>
#include <fstream>
#include <vector>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <elf.h>
#include "cpu.hpp"

class Emulator {
    public:
        Cpu cpu;
        uint32_t sp = 0;
        uint32_t pc = 0;

        Emulator(size_t mem_size = 1024 * 1024) : cpu(mem_size) {}

        void startCpu(void);
        void load_elf(const std::string& path);
};
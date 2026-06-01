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

        static constexpr uint32_t FLASH_BASE = 0x00000000;
        static constexpr uint32_t FLASH_SIZE = 64 * 1024;

        static constexpr uint32_t RAM_BASE   = 0x20000000;
        static constexpr uint32_t RAM_SIZE   = 16 * 1024;

        Emulator() : cpu(FLASH_SIZE, RAM_SIZE) {}

        void startCpu(void);
        void zero_memory(uint32_t addr, size_t size);
        void load_elf(const std::string& path);
        void write_block(uint32_t addr,
                    const uint8_t* data,
                    size_t size);
};
#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <stdexcept>

#include <elf.h>

class Emulator
{
public:

    static constexpr uint32_t FLASH_BASE = 0x08000000;
    static constexpr uint32_t FLASH_SIZE = 64 * 1024;

    static constexpr uint32_t RAM_BASE   = 0x20000000;
    static constexpr uint32_t RAM_SIZE   = 16 * 1024;

    std::vector<uint8_t> flash;
    std::vector<uint8_t> ram;

    uint32_t regs[16]{};

public:

    Emulator()
        : flash(FLASH_SIZE),
          ram(RAM_SIZE)
    {
    }

    /* =========================
       MEMORY ACCESS
       ========================= */

    uint8_t read8(uint32_t addr)
    {
        if (addr >= FLASH_BASE &&
            addr < FLASH_BASE + flash.size())
        {
            return flash[addr - FLASH_BASE];
        }

        if (addr >= RAM_BASE &&
            addr < RAM_BASE + ram.size())
        {
            return ram[addr - RAM_BASE];
        }

        throw std::runtime_error("Invalid read8");
    }

    uint16_t read16(uint32_t addr)
    {
        return
            read8(addr) |
            (read8(addr + 1) << 8);
    }

    uint32_t read32(uint32_t addr)
    {
        return
            read8(addr) |
            (read8(addr + 1) << 8) |
            (read8(addr + 2) << 16) |
            (read8(addr + 3) << 24);
    }

    void write8(uint32_t addr, uint8_t val)
    {
        if (addr >= RAM_BASE &&
            addr < RAM_BASE + ram.size())
        {
            ram[addr - RAM_BASE] = val;
            return;
        }

        throw std::runtime_error("Invalid write8");
    }

    void write_block(uint32_t addr,
                     const uint8_t* data,
                     size_t size)
    {
        /* FLASH */
        if (addr >= FLASH_BASE &&
            addr + size <= FLASH_BASE + flash.size())
        {
            memcpy(&flash[addr - FLASH_BASE],
                   data,
                   size);

            return;
        }

        /* RAM */
        if (addr >= RAM_BASE &&
            addr + size <= RAM_BASE + ram.size())
        {
            memcpy(&ram[addr - RAM_BASE],
                   data,
                   size);

            return;
        }

        throw std::runtime_error("Invalid block write");
    }

    void zero_memory(uint32_t addr, size_t size)
    {
        for (size_t i = 0; i < size; i++)
            write8(addr + i, 0);
    }

    /* =========================
       ELF LOADER
       ========================= */

    void load_elf(const std::string& path)
    {
        std::ifstream file(path, std::ios::binary);

        if (!file)
            throw std::runtime_error("Failed to open ELF");

        Elf32_Ehdr ehdr{};

        file.read(reinterpret_cast<char*>(&ehdr),
                  sizeof(ehdr));

        if (memcmp(ehdr.e_ident,
                   ELFMAG,
                   SELFMAG) != 0)
        {
            throw std::runtime_error("Not ELF");
        }

        if (ehdr.e_ident[EI_CLASS] != ELFCLASS32)
            throw std::runtime_error("Not ELF32");

        if (ehdr.e_machine != EM_ARM)
            throw std::runtime_error("Not ARM ELF");

        std::cout << "Valid ARM ELF\n";

        /* Go to program headers */
        file.seekg(ehdr.e_phoff);

        for (int i = 0; i < ehdr.e_phnum; i++)
        {
            Elf32_Phdr phdr{};

            file.read(reinterpret_cast<char*>(&phdr),
                      sizeof(phdr));

            if (phdr.p_type != PT_LOAD)
                continue;

            uint32_t addr = phdr.p_paddr;

            std::cout
                << "LOAD segment "
                << " addr=0x" << std::hex << addr
                << " filesz=" << std::dec
                << phdr.p_filesz
                << " memsz=" << phdr.p_memsz
                << "\n";

            /* Read segment data */
            std::vector<uint8_t> buffer(phdr.p_filesz);

            std::streampos current = file.tellg();

            file.seekg(phdr.p_offset);

            file.read(reinterpret_cast<char*>(buffer.data()),
                      phdr.p_filesz);

            if (!file)
                throw std::runtime_error("Segment read failed");

            /* Copy segment into emulated memory */
            write_block(addr,
                        buffer.data(),
                        phdr.p_filesz);

            /* Zero-fill BSS */
            if (phdr.p_memsz > phdr.p_filesz)
            {
                zero_memory(
                    addr + phdr.p_filesz,
                    phdr.p_memsz - phdr.p_filesz
                );
            }

            file.seekg(current);
        }

        reset();
    }

    /* =========================
       CPU RESET
       ========================= */

    void reset()
    {
        regs[13] = read32(FLASH_BASE + 0);

        regs[15] =
            read32(FLASH_BASE + 4) & ~1;

        std::cout << "SP = 0x"
                  << std::hex
                  << regs[13]
                  << "\n";

        std::cout << "PC = 0x"
                  << std::hex
                  << regs[15]
                  << "\n";
    }

    /* =========================
       FETCH
       ========================= */

    uint16_t fetch16()
    {
        uint16_t instr = read16(regs[15]);

        regs[15] += 2;

        return instr;
    }

    /* =========================
       RUN
       ========================= */

    void run()
    {
        while (true)
        {
            uint16_t instr = fetch16();

            std::cout
                << "PC=0x"
                << std::hex
                << regs[15] - 2
                << " INSTR=0x"
                << instr
                << "\n";

            /* TODO:
               decode + execute
            */

            break;
        }
    }
};
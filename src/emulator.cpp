#include "emulator.hpp"

enum CpuState
{
    FETCH,
    DECODE,
    EXECUTE,
    CPU_EXCEPTION_CHECK,
    CPU_EXCEPTION_ENTRY,
    CPU_EXCEPTION_HANDLER,
    CPU_EXCEPTION_RETURN,
    CPU_HALTED
};


void Emulator::startCpu(void)
{


    // for(uint16_t i = 0; i < 1000; i+=2)
    // {
    //     auto val = cpu.read16(i);

    //     printf("PC: %d: instruction: %x\n", i, val);
    // }

    cpu.cycle = 0;
    FILE* file = fopen("output.log", "w");
    cpu.output_file = file;
    
    while(true)
    {
        cpu.step();
    }

    fclose(file);
}

void Emulator::write_block(uint32_t addr,
                            const uint8_t* data,
                    size_t size)
{
    /* FLASH */
    if (addr >= FLASH_BASE &&
        addr + size <= FLASH_BASE + cpu.flash.size())
    {
        std::cout << "flash" << std::endl;

        std::memcpy(&cpu.flash[addr - FLASH_BASE],
                data,
                size);

        return;
    }

    /* RAM */
    if (addr >= RAM_BASE &&
        addr + size <= RAM_BASE + cpu.ram.size())
    {
         std::cout << "Ram" << std::endl;
        std::memcpy(&cpu.ram[addr - RAM_BASE],
                data,
                size);

        return;
    }

    throw std::runtime_error("Invalid block write");
}

void Emulator::zero_memory(uint32_t addr, size_t size)
{
    for (size_t i = 0; i < size; i++)
    {
        cpu.write8(addr + i, 0);
    }
}

void Emulator::load_elf(const std::string& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file)
        throw std::runtime_error("Failed to open ELF");

    Elf32_Ehdr ehdr{};
    file.read(reinterpret_cast<char*>(&ehdr), sizeof(ehdr));

    if (!file)
        throw std::runtime_error("Failed reading ELF header");

    if (std::memcmp(ehdr.e_ident, ELFMAG, SELFMAG) != 0)
        throw std::runtime_error("Not an ELF file");

    if (ehdr.e_ident[EI_CLASS] != ELFCLASS32)
        throw std::runtime_error("Not ELF32");

    if (ehdr.e_machine != EM_ARM)
        throw std::runtime_error("Not ARM ELF");

    std::cout << "Entry point: 0x" << std::hex << ehdr.e_entry << "\n";

    file.seekg(ehdr.e_phoff);

    for (int i = 0; i < ehdr.e_phnum; i++)
    {
        Elf32_Phdr phdr{};
        file.read(reinterpret_cast<char*>(&phdr), sizeof(phdr));

        if (!file)
            throw std::runtime_error("Failed reading program header");

        if (phdr.p_type != PT_LOAD)
            continue;

        // Use physical address for embedded
        uint32_t addr = phdr.p_paddr;

        printf("addr=%08x filesz=%x memsz=%x\n",
            addr,
            phdr.p_filesz,
            phdr.p_memsz);
        std::cout << "Loading segment " << i
                  << " addr=0x" << std::hex << addr
                  << " filesz=" << std::dec << phdr.p_filesz
                  << " memsz=" << phdr.p_memsz << "\n";

        // if (addr + phdr.p_memsz > cpu.memory.size())
            // throw std::runtime_error("Segment exceeds memory");

        // Save current file position
        std::streampos current = file.tellg();

        file.seekg(phdr.p_offset);

        std::vector<uint8_t> buffer(phdr.p_filesz);

        file.read(reinterpret_cast<char*>(buffer.data()), phdr.p_filesz);

        if (!file)
            throw std::runtime_error("Failed reading segment data");

        write_block(addr, buffer.data(), phdr.p_filesz);
        //Zero-fill (.bss)
        if (phdr.p_memsz > phdr.p_filesz)
        {
            zero_memory(addr + phdr.p_filesz, phdr.p_memsz - phdr.p_filesz);
        }

        // Restore position
        file.seekg(current);
    }

    cpu.regs[13] = cpu.read32Flash(0x0);

    // Part of the reset behaviour but it is elf loading section (FIX ME)
    cpu.msp = cpu.read32Flash(0x0);
    cpu.regs[15] = cpu.read32Flash(0x4) & ~1;
    cpu.fetch_pc = cpu.read32Flash(0x4) & ~1;

    std::cout << "Initial SP: 0x" << std::hex << cpu.regs[13] << "\n";
    std::cout << "Reset PC : 0x" << std::hex << cpu.regs[15] << "\n";
}
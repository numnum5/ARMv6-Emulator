#include "emulator.hpp"

void Emulator::startCpu(void)
{
    for (uint32_t i = 0; i < 10; i++)
    {
        cpu.decode();
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

        std::cout << "Loading segment " << i
                  << " addr=0x" << std::hex << addr
                  << " filesz=" << std::dec << phdr.p_filesz
                  << " memsz=" << phdr.p_memsz << "\n";

        if (addr + phdr.p_memsz > cpu.memory.size())
            throw std::runtime_error("Segment exceeds memory");

        // Save current file position
        std::streampos current = file.tellg();

        file.seekg(phdr.p_offset);
        file.read(reinterpret_cast<char*>(&cpu.memory[addr]),
                  phdr.p_filesz);

        if (!file)
            throw std::runtime_error("Failed reading segment data");

        // Zero-fill (.bss)
        if (phdr.p_memsz > phdr.p_filesz)
        {
            std::memset(&cpu.memory[addr + phdr.p_filesz],
                        0,
                        phdr.p_memsz - phdr.p_filesz);
        }

        // Restore position
        file.seekg(current);
    }

    
        uint16_t address = 0;
    for (int i = 0; i < 10; i++)
    {

        auto val = cpu.read16(address);

        printf("val: %x\n", val);
        address += 2;
    }


    cpu.regs[13] = cpu.memory.size();
    cpu.regs[15] = ehdr.e_entry & ~1;

    std::cout << "Initial SP: 0x" << std::hex << cpu.regs[13] << "\n";
    std::cout << "Reset PC : 0x" << std::hex << cpu.regs[15] << "\n";
}
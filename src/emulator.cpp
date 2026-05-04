#include "emulator.hpp"

void Emulator::startCpu(void)
{
    for (uint32_t i = 0; i < 8; i++)
    {
        cpu.decode();
    }
}

void Emulator::load_elf(const std::string& path) 
{
    std::ifstream file(path, std::ios::binary);
    if (!file)
        throw std::runtime_error("Failed to open ELF");

    // Read ELF header
    Elf32_Ehdr ehdr{};
    file.read(reinterpret_cast<char*>(&ehdr), sizeof(ehdr));

    if (!file)
        throw std::runtime_error("Failed reading ELF header");

    // Validate ELF magic
    if (std::memcmp(ehdr.e_ident, ELFMAG, SELFMAG) != 0)
        throw std::runtime_error("Not an ELF file");

    if (ehdr.e_ident[EI_CLASS] != ELFCLASS32)
        throw std::runtime_error("Not ELF32");

    if (ehdr.e_machine != EM_ARM)
        throw std::runtime_error("Not ARM ELF");

    std::cout << "Entry point: 0x" << std::hex << ehdr.e_entry << "\n";

    // Read program headers
    file.seekg(ehdr.e_phoff);

    for (int i = 0; i < ehdr.e_phnum; i++) 
    {
        Elf32_Phdr phdr{};
        file.read(reinterpret_cast<char*>(&phdr), sizeof(phdr));

        if (phdr.p_type != PT_LOAD)
            continue;

        std::cout << "Loading segment " << i
                    << " addr=0x" << std::hex << phdr.p_vaddr
                    << " filesz=" << std::dec << phdr.p_filesz
                    << " memsz=" << phdr.p_memsz << "\n";

        if (phdr.p_vaddr + phdr.p_memsz > cpu.memory.size())
            throw std::runtime_error("Segment exceeds memory");

        std::streampos current = file.tellg();

        // Read segment data
        file.seekg(phdr.p_offset);
        file.read(reinterpret_cast<char*>(&cpu.memory[phdr.p_vaddr]),
                    phdr.p_filesz);

        // Zero-fill .bss region
        if (phdr.p_memsz > phdr.p_filesz) {
            std::memset(&cpu.memory[phdr.p_vaddr + phdr.p_filesz],
                        0,
                        phdr.p_memsz - phdr.p_filesz);
        }

        file.seekg(current);
    }

    // Cortex-M boot from vector table
    sp = cpu.read32(0x00000000);
    pc = cpu.read32(0x00000004);

    std::cout << "Initial SP: 0x" << std::hex << sp << "\n";
    std::cout << "Reset PC : 0x" << std::hex << pc << "\n";
}
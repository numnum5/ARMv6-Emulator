#include <iostream>
#include "emulator.hpp"
#include "compiler.hpp"


int main(void)
{
    auto filename = std::string("test");
    compile(filename);

    Emulator emu;

    emu.load_elf("../build/main.elf");


    emu.startCpu();


//     // Store opcodes into memory (little-endian)
// uint16_t program[] =
// {
//     // addr 0x0000
//     0x200A, // MOVS r0,#10
//     0x2102, // MOVS r1,#2

//     // addr 0x0004
//     0b0000000001000010, // LSLS r2,r0,#1
//     0x0843, // LSRS r3,r0,#1

//     // addr 0x0008
//     0x1844, // ADDS r4,r0,r1
//     0x1A45, // SUBS r5,r0,r1

//     // addr 0x000C
//     0x4288, // CMP r0,r1
//     0xD001, // BEQ skip

//     // addr 0x0010
//     0x2601, // MOVS r6,#1
//     0xE002, // B end

//     // addr 0x0014 (skip)
//     0x2600, // MOVS r6,#0
//     0xBF00, // NOP

//     // addr 0x0018 (end)
//     0x6008, // STR r0,[r1,#0]
//     0x680F, // LDR r7,[r1,#0]

//     // addr 0x001C
//     0xB403, // PUSH {r0,r1}
//     0xBC0C, // POP {r2,r3}

//     // addr 0x0020
//     0x4048, // ANDS r0,r1
//     0x430A, // ORRS r2,r1

//     // addr 0x0024
//     0x4059, // EORS r1,r3
//     0xE7FE  // B .
// };

//     for (int i = 0; i < sizeof(program)/sizeof(program[0]); i++)
//     {
//         uint16_t instr = program[i];
//         printf("%d\n", classify(instr));
//         cpu.flash[i * 2 + 0] = instr & 0xFF;        // low byte
//         cpu.flash[i * 2 + 1] = (instr >> 8) & 0xFF; // high byte
//     }
    
//     printf("%LX\n", cpu.flash[0]);
//         // Print memory contents
//     std::cout << "Loaded machine code:\n";

//     for (uint32_t i = 0; i < 8; i++)
//     {
//         cpu.decode();
//     }

    // printf("%d\n", test2.count());
  
    // cpu.decode(0100000001001000);
}
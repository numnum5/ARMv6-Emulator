#include <catch2/catch.hpp>

#include "cpu.hpp"

// TEST_CASE("STR_IMM stores word")
// {
//     Cpu cpu(0x1000, 0x1000);
//     //cpu.reset();

//     cpu.regs[0] = 0x20000050;
//     cpu.fetch_pc = 0x40;

//     cpu.writeFlash16(0x40,     0x212A); // movs r1,#42
//     cpu.writeFlash16(0x40 + 2, 0x6041); // str r1,[r0,#4]

//     cpu.test(5);

//     REQUIRE(cpu.read32(0x20000054) == 42);
//     REQUIRE(cpu.cycle == 5);
// }

TEST_CASE("STRB_IMM stores byte")
{
    Cpu cpu(0x1000, 0x1000);
    cpu.reset();

    cpu.regs[0] = 0x20000050;
    cpu.fetch_pc = 0x40;

    cpu.writeFlash16(0x40,     0x214D); // movs r1,#77
    cpu.writeFlash16(0x40 + 2, 0x7081); // strb r1,[r0,#2]

    cpu.test(5);

    REQUIRE(cpu.read8(0x20000052) == 77);
    REQUIRE(cpu.cycle == 5);
}
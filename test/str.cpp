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

TEST_CASE("STRB_REG stores byte")
{
    Cpu cpu(0x1000, 0x1000);
    cpu.reset();
    cpu.regs[0] = 0x20000050;
    cpu.fetch_pc = 0x40;

    cpu.writeFlash16(0x40,     0x2163); // movs r1,#99
    cpu.writeFlash16(0x40 + 2, 0x2203); // movs r2,#3
    cpu.writeFlash16(0x40 + 4, 0x5481); // strb r1,[r0,r2]

    cpu.test(6);

    REQUIRE(cpu.read8(0x20000053) == 99);
    REQUIRE(cpu.cycle == 6);
}

TEST_CASE("STRH_IMM stores halfword")
{
    Cpu cpu(0x1000, 0x1000);
    cpu.reset();

    cpu.regs[0] = 0x20000050;
    cpu.fetch_pc = 0x40;

    cpu.writeFlash16(0x40,     0x2158); // movs r1,#88
    cpu.writeFlash16(0x40 + 2, 0x8041); // strh r1,[r0,#2]

    cpu.test(5);

    REQUIRE(cpu.read16(0x20000052) == 88);
    REQUIRE(cpu.cycle == 5);
}

TEST_CASE("STRH_REG stores halfword")
{
    Cpu cpu(0x1000, 0x1000);
    cpu.reset();

    cpu.regs[0] = 0x20000050;
    cpu.fetch_pc = 0x40;

    cpu.writeFlash16(0x40,     0x217B); // movs r1,#123
    cpu.writeFlash16(0x40 + 2, 0x2206); // movs r2,#6
    cpu.writeFlash16(0x40 + 4, 0x5281); // strh r1,[r0,r2]

    cpu.test(6);

    REQUIRE(cpu.read16(0x20000056) == 123);
    REQUIRE(cpu.cycle == 6);
}

TEST_CASE("STMIA stores multiple registers and updates base")
{
    Cpu cpu(0x1000, 0x1000);
    cpu.reset();

    cpu.regs[0] = 0x20000050;
    cpu.fetch_pc = 0x40;

    cpu.writeFlash16(0x40,     0x2105); // movs r1,#5
    cpu.writeFlash16(0x40 + 2, 0x220A); // movs r2,#10
    cpu.writeFlash16(0x40 + 4, 0xC006); // stmia r0!, {r1,r2}

    cpu.test(7);

    REQUIRE(cpu.read32(0x20000050) == 5);
    REQUIRE(cpu.read32(0x20000054) == 10);

    REQUIRE(cpu.regs[0] == 0x20000058);
    REQUIRE(cpu.cycle == 7);
}
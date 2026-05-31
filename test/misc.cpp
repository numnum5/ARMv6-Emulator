#include <catch2/catch.hpp>

#include "cpu.hpp"

TEST_CASE("PUSH stores registers and updates SP")
{
    Cpu cpu(0x1000, 0x1000);

    cpu.fetch_pc = 0x40;

    // Initial stack pointer
    cpu.regs[13] = 0x20000100;

    cpu.regs[1] = 5;
    cpu.regs[2] = 10;

    cpu.writeFlash16(0x40, 0xB406); // push {r1,r2}

    cpu.test(5);

    // SP -= 8
    // [SP+0] = r1
    // [SP+4] = r2
    REQUIRE(cpu.read32(0x200000F8) == 5);
    REQUIRE(cpu.read32(0x200000FC) == 10);

    REQUIRE(cpu.regs[13] == 0x200000F8);
    REQUIRE(cpu.cycle == 5);
}

TEST_CASE("POP loads registers and updates SP")
{
    Cpu cpu(0x1000, 0x1000);

    cpu.fetch_pc = 0x40;

    // Initial stack pointer
    cpu.regs[13] = 0x200000F8;

    // Stack contents
    cpu.write32(0x200000F8, 5);
    cpu.write32(0x200000FC, 10);

    cpu.writeFlash16(0x40, 0xBC06); // pop {r1,r2}

    cpu.test(5);

    REQUIRE(cpu.regs[1] == 5);
    REQUIRE(cpu.regs[2] == 10);

    // SP += 8
    REQUIRE(cpu.regs[13] == 0x20000100);
    REQUIRE(cpu.cycle == 5);
}

TEST_CASE("POP loads registers and PC and updates SP")
{
    Cpu cpu(0x1000, 0x1000);

    cpu.fetch_pc = 0x40;

    // Initial stack pointer
    cpu.regs[13] = 0x200000F8;

    // Stack contents
    cpu.write32(0x200000F8, 5);          // r1
    cpu.write32(0x200000FC, 0x00000101); // pc (Thumb bit set)

    cpu.writeFlash16(0x40, 0xBD02); // pop {r1, pc}

    cpu.test(5);

    REQUIRE(cpu.regs[1] == 5);

    // PC loaded from stack
    REQUIRE(cpu.fetch_pc == 0x00000100); 

    // SP += 8
    REQUIRE(cpu.regs[13] == 0x20000100);

    REQUIRE(cpu.cycle == 5);
}
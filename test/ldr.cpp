#include <catch2/catch.hpp>

#include "cpu.hpp"

TEST_CASE("LDR_IMM loads word")
{
    Cpu cpu(0x1000, 0x1000);

    cpu.regs[0] = 0x20000050;
    cpu.fetch_pc = 0x40;

    cpu.write32(0x20000054, 42);

    cpu.writeFlash16(0x40, 0x6841); // ldr r1,[r0,#4]

    cpu.test(4);

    REQUIRE(cpu.regs[1] == 42);
    REQUIRE(cpu.regs[0] == 0x20000050);
    REQUIRE(cpu.cycle == 4);
}

TEST_CASE("LDR_REG loads word")
{
    Cpu cpu(0x1000, 0x1000);

    cpu.regs[0] = 0x20000050;
    cpu.regs[2] = 4;
    cpu.fetch_pc = 0x40;

    cpu.write32(0x20000054, 99);

    cpu.writeFlash16(0x40, 0x5881); // ldr r1,[r0,r2]

    cpu.test(4);

    REQUIRE(cpu.regs[1] == 99);
    REQUIRE(cpu.cycle == 4);
}

TEST_CASE("LDR_LITERAL loads word")
{
    Cpu cpu(0x1000, 0x1000);

    cpu.fetch_pc = 0x40;

    // PC = Align(0x40 + 4, 4) = 0x44
    // imm8 = 1 -> offset = 4
    cpu.writeFlash32(0x44, 1234);
    cpu.writeFlash16(0x40, 0x4900); // ldr r1,[pc,#4]

    cpu.test(4);

    REQUIRE(cpu.regs[1] == 1234);
    REQUIRE(cpu.cycle == 4);
}

TEST_CASE("LDRB_IMM loads byte")
{
    Cpu cpu(0x1000, 0x1000);

    cpu.regs[0] = 0x20000050;
    cpu.fetch_pc = 0x40;

    cpu.write8(0x20000052, 77);

    cpu.writeFlash16(0x40, 0x7881); // ldrb r1,[r0,#2]

    cpu.test(4);

    REQUIRE(cpu.regs[1] == 77);
    REQUIRE(cpu.cycle == 4);
}

TEST_CASE("LDRB_REG loads byte")
{
    Cpu cpu(0x1000, 0x1000);

    cpu.regs[0] = 0x20000050;
    cpu.regs[2] = 3;
    cpu.fetch_pc = 0x40;

    cpu.write8(0x20000053, 88);

    cpu.writeFlash16(0x40, 0x5C81); // ldrb r1,[r0,r2]

    cpu.test(4);

    REQUIRE(cpu.regs[1] == 88);
    REQUIRE(cpu.cycle == 4);
}

TEST_CASE("LDRH_IMM loads halfword")
{
    Cpu cpu(0x1000, 0x1000);

    cpu.regs[0] = 0x20000050;
    cpu.fetch_pc = 0x40;

    cpu.write16(0x20000052, 123);
    cpu.write16(0x20000052 + 2, 0x000); // -32768
    cpu.write16(0x20000052 + 4, 0x000); // -32768

    cpu.writeFlash16(0x40, 0x8841); // ldrh r1,[r0,#2]

    cpu.test(4);

    REQUIRE(cpu.regs[1] == 123);
    REQUIRE(cpu.cycle == 4);
}

TEST_CASE("LDRH_REG loads halfword")
{
    Cpu cpu(0x1000, 0x1000);

    cpu.regs[0] = 0x20000050;
    cpu.regs[2] = 6;
    cpu.fetch_pc = 0x40;

    cpu.write16(0x20000056, 456);

    cpu.writeFlash16(0x40, 0x5A81); // ldrh r1,[r0,r2]

    cpu.test(4);

    REQUIRE(cpu.regs[1] == 456);
    REQUIRE(cpu.cycle == 4);
}

TEST_CASE("LDRSB_REG sign extends byte")
{
    Cpu cpu(0x1000, 0x1000);

    cpu.regs[0] = 0x20000050;
    cpu.regs[2] = 1;
    cpu.fetch_pc = 0x40;

    cpu.write8(0x20000051, 0x80); // -128

    cpu.writeFlash16(0x40, 0x5681); // ldrsb r1,[r0,r2]

    cpu.test(4);

    REQUIRE(cpu.regs[1] == 0xFFFFFF80);
    REQUIRE(cpu.cycle == 4);
}

TEST_CASE("LDRSH_REG sign extends halfword")
{
    Cpu cpu(0x1000, 0x1000);

    cpu.regs[0] = 0x20000050;
    cpu.regs[2] = 2;
    cpu.fetch_pc = 0x40;

    cpu.write16(0x20000052, 0x8000); // -32768
    cpu.write16(0x20000052 + 2, 0x000); // -32768
    cpu.write16(0x20000052 + 4, 0x000); // -32768

    cpu.writeFlash16(0x40, 0x5E81); // ldrsh r1,[r0,r2]

    cpu.test(4);

    REQUIRE(cpu.regs[1] == 0xFFFF8000);
    REQUIRE(cpu.cycle == 4);
}

TEST_CASE("LDMIA loads multiple registers and updates base")
{
    Cpu cpu(0x1000, 0x1000);
    cpu.reset();

    cpu.regs[0] = 0x20000050;
    cpu.fetch_pc = 0x40;

    // Memory contents to load
    cpu.write32(0x20000050, 5);
    cpu.write32(0x20000054, 10);

    cpu.writeFlash16(0x40, 0xC806); // ldmia r0!, {r1,r2}

    cpu.test(5);

    REQUIRE(cpu.regs[1] == 5);
    REQUIRE(cpu.regs[2] == 10);

    // Base register incremented by 2 words
    REQUIRE(cpu.regs[0] == 0x20000058);

    REQUIRE(cpu.cycle == 5);
}
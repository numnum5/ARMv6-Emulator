#include <catch2/catch_test_macros.hpp>

#include "cpu.hpp"

TEST_CASE("ALU instructions")
{
    SECTION("AND")
    {
        Cpu cpu(0x1000, 0x1000);

        cpu.fetch_pc = 0x40;
        cpu.regs[0] = 0b1100;
        cpu.regs[1] = 0b1010;

        cpu.xpsr.setC(false);
        cpu.xpsr.setN(false);
        cpu.xpsr.setV(false);
        cpu.xpsr.setZ(false);

        cpu.writeFlash16(0x40, 0x4008); // ands r0,r1

        cpu.test(3);

        REQUIRE(cpu.regs[0] == 0b1000);
        REQUIRE(cpu.cycle == 3);
    }

    SECTION("EOR")
    {
        Cpu cpu(0x1000, 0x1000);

        cpu.fetch_pc = 0x40;
        cpu.regs[0] = 0b1100;
        cpu.regs[1] = 0b1010;

        cpu.xpsr.setC(false);
        cpu.xpsr.setN(false);
        cpu.xpsr.setV(false);
        cpu.xpsr.setZ(false);

        cpu.writeFlash16(0x40, 0x4048); // eors r0,r1

        cpu.test(3);

        REQUIRE(cpu.regs[0] == 0b0110);
    }

    SECTION("LSL register")
    {
        Cpu cpu(0x1000, 0x1000);

        cpu.fetch_pc = 0x40;
        cpu.regs[0] = 3;
        cpu.regs[1] = 2;

        cpu.xpsr.setC(false);
        cpu.xpsr.setN(false);
        cpu.xpsr.setV(false);
        cpu.xpsr.setZ(false);

        cpu.writeFlash16(0x40, 0x4088); // lsls r0,r1

        cpu.test(3);

        REQUIRE(cpu.regs[0] == 12);
    }

    SECTION("LSR register")
    {
        Cpu cpu(0x1000, 0x1000);

        cpu.fetch_pc = 0x40;
        cpu.regs[0] = 16;
        cpu.regs[1] = 2;

        cpu.xpsr.setC(false);
        cpu.xpsr.setN(false);
        cpu.xpsr.setV(false);
        cpu.xpsr.setZ(false);

        cpu.writeFlash16(0x40, 0x40C8); // lsrs r0,r1

        cpu.test(3);

        REQUIRE(cpu.regs[0] == 4);
    }

    SECTION("ASR register")
    {
        Cpu cpu(0x1000, 0x1000);

        cpu.fetch_pc = 0x40;
        cpu.regs[0] = static_cast<uint32_t>(-16);
        cpu.regs[1] = 2;

        cpu.xpsr.setC(false);
        cpu.xpsr.setN(false);
        cpu.xpsr.setV(false);
        cpu.xpsr.setZ(false);

        cpu.writeFlash16(0x40, 0x4108); // asrs r0,r1

        cpu.test(3);

        REQUIRE(static_cast<int32_t>(cpu.regs[0]) == -4);
    }

    SECTION("ADC")
    {
        Cpu cpu(0x1000, 0x1000);

        cpu.fetch_pc = 0x40;
        cpu.regs[0] = 10;
        cpu.regs[1] = 5;

        cpu.xpsr.setC(true);
        cpu.xpsr.setN(false);
        cpu.xpsr.setV(false);
        cpu.xpsr.setZ(false);

        cpu.writeFlash16(0x40, 0x4148); // adcs r0,r1

        cpu.test(3);

        REQUIRE(cpu.regs[0] == 16);
    }

    SECTION("SBC")
    {
        Cpu cpu(0x1000, 0x1000);

        cpu.fetch_pc = 0x40;
        cpu.regs[0] = 10;
        cpu.regs[1] = 3;

        cpu.xpsr.setC(true);
        cpu.xpsr.setN(false);
        cpu.xpsr.setV(false);
        cpu.xpsr.setZ(false);

        cpu.writeFlash16(0x40, 0x4188); // sbcs r0,r1

        cpu.test(3);

        REQUIRE(cpu.regs[0] == 7);
    }

    SECTION("ROR")
    {
        Cpu cpu(0x1000, 0x1000);

        cpu.fetch_pc = 0x40;
        cpu.regs[0] = 0x80000000;
        cpu.regs[1] = 1;

        cpu.xpsr.setC(false);
        cpu.xpsr.setN(false);
        cpu.xpsr.setV(false);
        cpu.xpsr.setZ(false);

        cpu.writeFlash16(0x40, 0x41C8); // rors r0,r1

        cpu.test(3);

        REQUIRE(cpu.regs[0] == 0x40000000);
    }

    SECTION("TST")
    {
        Cpu cpu(0x1000, 0x1000);

        cpu.fetch_pc = 0x40;
        cpu.regs[0] = 0b1100;
        cpu.regs[1] = 0b0011;

        cpu.xpsr.setC(false);
        cpu.xpsr.setN(false);
        cpu.xpsr.setV(false);
        cpu.xpsr.setZ(false);

        cpu.writeFlash16(0x40, 0x4208); // tst r0,r1

        cpu.test(3);

        REQUIRE(cpu.regs[0] == 0b1100);
        REQUIRE(cpu.xpsr.Z() == true);
    }

    SECTION("NEG")
    {
        Cpu cpu(0x1000, 0x1000);

        cpu.fetch_pc = 0x40;
        cpu.regs[1] = 5;

        cpu.xpsr.setC(false);
        cpu.xpsr.setN(false);
        cpu.xpsr.setV(false);
        cpu.xpsr.setZ(false);

        cpu.writeFlash16(0x40, 0x4248); // negs r0,r1

        cpu.test(3);

        REQUIRE(static_cast<int32_t>(cpu.regs[0]) == -5);
    }

    SECTION("CMP")
    {
        Cpu cpu(0x1000, 0x1000);

        cpu.fetch_pc = 0x40;
        cpu.regs[0] = 10;
        cpu.regs[1] = 10;

        cpu.xpsr.setC(false);
        cpu.xpsr.setN(false);
        cpu.xpsr.setV(false);
        cpu.xpsr.setZ(false);

        cpu.writeFlash16(0x40, 0x4288); // cmp r0,r1

        cpu.test(3);

        REQUIRE(cpu.xpsr.Z() == true);
        REQUIRE(cpu.regs[0] == 10);
    }

    SECTION("CMN")
    {
        Cpu cpu(0x1000, 0x1000);

        cpu.fetch_pc = 0x40;
        cpu.regs[0] = 1;
        cpu.regs[1] = 0xFFFFFFFF;

        cpu.xpsr.setC(false);
        cpu.xpsr.setN(false);
        cpu.xpsr.setV(false);
        cpu.xpsr.setZ(false);

        cpu.writeFlash16(0x40, 0x42C8); // cmn r0,r1

        cpu.test(3);

        REQUIRE(cpu.xpsr.Z() == true);
    }

    SECTION("ORR")
    {
        Cpu cpu(0x1000, 0x1000);

        cpu.fetch_pc = 0x40;
        cpu.regs[0] = 0b1000;
        cpu.regs[1] = 0b0011;

        cpu.xpsr.setC(false);
        cpu.xpsr.setN(false);
        cpu.xpsr.setV(false);
        cpu.xpsr.setZ(false);

        cpu.writeFlash16(0x40, 0x4308); // orrs r0,r1

        cpu.test(3);

        REQUIRE(cpu.regs[0] == 0b1011);
    }

    SECTION("MUL")
    {
        Cpu cpu(0x1000, 0x1000);

        cpu.fetch_pc = 0x40;
        cpu.regs[0] = 6;
        cpu.regs[1] = 7;

        cpu.xpsr.setC(false);
        cpu.xpsr.setN(false);
        cpu.xpsr.setV(false);
        cpu.xpsr.setZ(false);

        cpu.writeFlash16(0x40, 0x4348); // muls r0,r1

        cpu.test(3);

        REQUIRE(cpu.regs[0] == 42);
    }

    SECTION("BIC")
    {
        Cpu cpu(0x1000, 0x1000);

        cpu.fetch_pc = 0x40;
        cpu.regs[0] = 0b1111;
        cpu.regs[1] = 0b0101;

        cpu.xpsr.setC(false);
        cpu.xpsr.setN(false);
        cpu.xpsr.setV(false);
        cpu.xpsr.setZ(false);

        cpu.writeFlash16(0x40, 0x4388); // bics r0,r1

        cpu.test(3);

        REQUIRE(cpu.regs[0] == 0b1010);
    }

    SECTION("MVN")
    {
        Cpu cpu(0x1000, 0x1000);

        cpu.fetch_pc = 0x40;
        cpu.regs[1] = 0x0F0F0F0F;

        cpu.xpsr.setC(false);
        cpu.xpsr.setN(false);
        cpu.xpsr.setV(false);
        cpu.xpsr.setZ(false);

        cpu.writeFlash16(0x40, 0x43C8); // mvns r0,r1

        cpu.test(3);

        REQUIRE(cpu.regs[0] == 0xF0F0F0F0);
    }
}
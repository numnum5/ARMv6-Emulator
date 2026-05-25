#include "cpu.hpp"


uint32_t Cpu::BXWritePC2(uint32_t address)
{
    if (currentMode == Mode::MODE_HANDLER && (address >> 28 & 0xF) == 0b1111)
    {
        std::cerr << "BXWritePC" << "exception return" << std::endl; 
        while(1);   
    //  ExceptionReturn(address<27:0>);
    }
    else
    {
        this->xpsr.setT(address & 0b1);
        return address & ~1u;
    }
}

void Cpu::print_mem()
{
    for (uint32_t addr = 0x20000fe0 - RAM_BASE;
         addr <= 0x20000fe8 - RAM_BASE;
         addr += 4)
    {
        uint32_t value =
            this->ram[addr] |
            (this->ram[addr + 1] << 8) |
            (this->ram[addr + 2] << 16) |
            (this->ram[addr + 3] << 24);

        printf("mem[%08x]: %08x\n",
               addr,
               value);
    }
}

void Cpu::execute_final(DecodeExecuteLatch& DE_latch,
                        DecodeExecuteLatch& next)
{
    switch (DE_latch.opcode)
    {
case Opcode::SUB_SP_T1:
{
    printf(
        "SUB_SP pc=%x before=%x imm=%u\n",
        DE_latch.pc,
        regs[13],
        DE_latch.imm32
    );

    printf("imm32: %d\n", DE_latch.imm32);

    regs[13] -= DE_latch.imm32;

    printf(
        "SUB_SP after=%x\n",
        regs[13]
    );

    break;
}

case Opcode::ADD_SP_T2:
{
    printf(
        "ADD_SP pc=%x before=%x imm=%u\n",
        DE_latch.pc,
        regs[13],
        DE_latch.imm32
    );

    printf("imm32: %d\n", DE_latch.imm32);

    regs[DE_latch.destination] += DE_latch.imm32;

    printf(
        "ADD_SP: after=%x\n",
        regs[13]
    );

    break;
}
        case Opcode::INVALID:
        {
            printf("Opcode::INVALID\n");
            break;
        }

        //
        // Branch / control flow
        //
        case Opcode::B:
        {
            this->branch_pc = DE_latch.pc + 4 + (int32_t)DE_latch.imm32;
            branch_taken = true;
            flush = true;

            printf("Opcode::B, next branch pc: %d\n", this->branch_pc);
            break;
        }

        case Opcode::B_COND:
        {
            if (conditionPassed(DE_latch.cond))
            {
                this->branch_pc =
                    DE_latch.pc + 4 + DE_latch.imm32;

                branch_taken = true;
                flush = true;
            }

            printf("Opcode::B_COND\n");
            break;
        }

        case Opcode::BL:
        {
            

            

            branch_taken = true;
            flush = true;

            uint32_t next = DE_latch.pc + 4;
            regs[14] = next | 1;
            this->branch_pc = DE_latch.pc + 4 + (int32_t)DE_latch.imm32;

            printf("Opcode::BL, imm: %x\n", DE_latch.imm32);
            break;
        }

        case Opcode::BLX:
        {
            regs[14] = (DE_latch.pc + 2) | 1;

           this->branch_pc =
                BXWritePC2(regs[DE_latch.m]);

            branch_taken = true;
            flush = true;

            printf("Opcode::BLX\n");
            break;
        }

        case Opcode::BX:
        {
            this->branch_pc =
                BXWritePC2(regs[DE_latch.m]);

            branch_taken = true;
            flush = true;

            printf("Opcode::BX\n");
            break;
        }

        //
        // Arithmetic / data processing
        //
        case Opcode::ADC:
        {
            auto result =
                addWithCarry(
                    regs[DE_latch.n],
                    regs[DE_latch.m],
                    xpsr.C()
                );

            regs[DE_latch.destination] =
                result.result;

            setAPSRValues(
                result.carry_out,
                (result.result >> 31) & 1,
                result.overflow,
                result.result == 0
            );

            printf("Opcode::ADC\n");
            break;
        }

        case Opcode::ADD_REG:
        {
            auto result =
                addWithCarry(
                    regs[DE_latch.n],
                    regs[DE_latch.m],
                    0
                );

            regs[DE_latch.destination] =
                result.result;

            setAPSRValues(
                result.carry_out,
                (result.result >> 31) & 1,
                result.overflow,
                result.result == 0
            );

            printf("Opcode::ADD_REG\n");
            break;
        }

        case Opcode::ADD_IMM_T1:
        case Opcode::ADD_IMM_T2:
        {
            auto result =
                addWithCarry(
                    regs[DE_latch.n],
                    DE_latch.imm32,
                    0
                );

            regs[DE_latch.destination] =
                result.result;

            setAPSRValues(
                result.carry_out,
                (result.result >> 31) & 1,
                result.overflow,
                result.result == 0
            );

            printf("Opcode::ADD_IMM\n");
            break;
        }

        case Opcode::ADD_SP_T1:
        {
             printf("SP: before ADD_SP: %x\n", regs[13]);
            regs[DE_latch.destination] = regs[13] + DE_latch.imm32;

            printf("SP: after ADD_SP: %x\n", regs[13]);

            printf("Opcode::ADD_SP_T1\n");
            break;
        }

        case Opcode::ADR:
        {
            uint32_t base =
                (DE_latch.pc + 4) & ~0x3;

            regs[DE_latch.destination] =
                base + DE_latch.imm32;

            printf("Opcode::ADR\n");
            break;
        }

        case Opcode::AND:
        {
            uint32_t result =
                regs[DE_latch.n] &
                regs[DE_latch.m];

            regs[DE_latch.destination] =
                result;

            setAPSRValues(
                xpsr.C(),
                (result >> 31) & 1,
                xpsr.V(),
                result == 0
            );

            printf("Opcode::AND\n");
            break;
        }

        case Opcode::ASR_IMM:
        {
            auto shifted =
                shift_c(
                    regs[DE_latch.m],
                    SRType_ASR,
                    DE_latch.shift_n,
                    xpsr.C()
                );

            regs[DE_latch.destination] =
                shifted.result;

            setAPSRValues(
                shifted.carry_out,
                (shifted.result >> 31) & 1,
                xpsr.V(),
                shifted.result == 0
            );

            printf("Opcode::ASR_IMM\n");
            break;
        }

        case Opcode::ASR_REG:
        {
            uint8_t shift_n =
                regs[DE_latch.m] & 0xFF;

            auto shifted =
                shift_c(
                    regs[DE_latch.n],
                    SRType_ASR,
                    shift_n,
                    xpsr.C()
                );

            regs[DE_latch.destination] =
                shifted.result;

            setAPSRValues(
                shifted.carry_out,
                (shifted.result >> 31) & 1,
                xpsr.V(),
                shifted.result == 0
            );

            printf("Opcode::ASR_REG\n");
            break;
        }

        case Opcode::BIC:
        {
            uint32_t result =
                regs[DE_latch.n] &
                ~regs[DE_latch.m];

            regs[DE_latch.destination] =
                result;

            setAPSRValues(
                xpsr.C(),
                (result >> 31) & 1,
                xpsr.V(),
                result == 0
            );

            printf("Opcode::BIC\n");
            break;
        }

        case Opcode::CMN:
        {
            auto result =
                addWithCarry(
                    regs[DE_latch.n],
                    regs[DE_latch.m],
                    0
                );

            setAPSRValues(
                result.carry_out,
                (result.result >> 31) & 1,
                result.overflow,
                result.result == 0
            );

            printf("Opcode::CMN\n");
            break;
        }

        case Opcode::CMP_REG:
        {
            auto result =
                addWithCarry(
                    regs[DE_latch.n],
                    ~regs[DE_latch.m],
                    1
                );

            setAPSRValues(
                result.carry_out,
                (result.result >> 31) & 1,
                result.overflow,
                result.result == 0
            );

            printf("Opcode::CMP_REG\n");
            break;
        }

        case Opcode::CMP_IMM:
        {
            auto result =
                addWithCarry(
                    regs[DE_latch.n],
                    ~DE_latch.imm32,
                    1
                );

            setAPSRValues(
                result.carry_out,
                (result.result >> 31) & 1,
                result.overflow,
                result.result == 0
            );

            printf("Opcode::CMP_IMM\n");
            break;
        }

        case Opcode::EOR:
        {
            uint32_t result =
                regs[DE_latch.n] ^
                regs[DE_latch.m];

            regs[DE_latch.destination] =
                result;

            setAPSRValues(
                xpsr.C(),
                (result >> 31) & 1,
                xpsr.V(),
                result == 0
            );

            printf("Opcode::EOR\n");
            break;
        }

        case Opcode::LSL_IMM:
        {

            printf("Opcode::LSL_IMM DE_latch.shift_n: %d\n", DE_latch.shift_n);
            auto shifted =
                shift_c(
                    regs[DE_latch.m],
                    SRType_LSL,
                    DE_latch.shift_n,
                    xpsr.C()
                );


            regs[DE_latch.destination] =
                shifted.result;

            setAPSRValues(
                shifted.carry_out,
                (shifted.result >> 31) & 1,
                xpsr.V(),
                shifted.result == 0
            );

            printf("Opcode::LSL_IMM\n");
            break;
        }

        case Opcode::LSL_REG:
        {
            uint8_t shift_n =
                regs[DE_latch.m] & 0xFF;

            auto shifted =
                shift_c(
                    regs[DE_latch.n],
                    SRType_LSL,
                    shift_n,
                    xpsr.C()
                );

            regs[DE_latch.destination] =
                shifted.result;

            setAPSRValues(
                shifted.carry_out,
                (shifted.result >> 31) & 1,
                xpsr.V(),
                shifted.result == 0
            );

            printf("Opcode::LSL_REG\n");
            break;
        }

        case Opcode::LSR_IMM:
        {
            auto shifted =
                shift_c(
                    regs[DE_latch.m],
                    SRType_LSR,
                    DE_latch.shift_n,
                    xpsr.C()
                );

            regs[DE_latch.destination] =
                shifted.result;

            setAPSRValues(
                shifted.carry_out,
                (shifted.result >> 31) & 1,
                xpsr.V(),
                shifted.result == 0
            );

            printf("Opcode::LSR_IMM\n");
            break;
        }

        case Opcode::LSR_REG:
        {
            uint8_t shift_n =
                regs[DE_latch.m] & 0xFF;

            auto shifted =
                shift_c(
                    regs[DE_latch.n],
                    SRType_LSR,
                    shift_n,
                    xpsr.C()
                );

            regs[DE_latch.destination] =
                shifted.result;

            setAPSRValues(
                shifted.carry_out,
                (shifted.result >> 31) & 1,
                xpsr.V(),
                shifted.result == 0
            );

            printf("Opcode::LSR_REG\n");
            break;
        }

        case Opcode::MOV_IMM:
        {
            regs[DE_latch.destination] = DE_latch.imm32;

            setAPSRValues(
                xpsr.C(),
                (regs[DE_latch.destination] >> 31) & 1,
                xpsr.V(),
                regs[DE_latch.destination] == 0
            );

            printf("Opcode::MOV_IMM\n");
            break;
        }

        case Opcode::MOV_REG:
        {
            regs[DE_latch.destination] =
                regs[DE_latch.m];

            uint32_t result =
                regs[DE_latch.destination];

            setAPSRValues(
                xpsr.C(),
                (result >> 31) & 1,
                xpsr.V(),
                result == 0
            );

            printf("Opcode::MOV_REG\n");
            break;
        }

        case Opcode::MUL:
        {
            uint32_t result =
                regs[DE_latch.n] *
                regs[DE_latch.m];

            regs[DE_latch.destination] =
                result;

            setAPSRValues(
                xpsr.C(),
                (result >> 31) & 1,
                xpsr.V(),
                result == 0
            );

            printf("Opcode::MUL\n");
            break;
        }

        case Opcode::MVN:
        {
            uint32_t result =
                ~regs[DE_latch.m];

            regs[DE_latch.destination] =
                result;

            setAPSRValues(
                xpsr.C(),
                (result >> 31) & 1,
                xpsr.V(),
                result == 0
            );

            printf("Opcode::MVN\n");
            break;
        }

        case Opcode::NEG:
        {
            auto result =
                addWithCarry(
                    ~regs[DE_latch.m],
                    0,
                    1
                );

            regs[DE_latch.destination] =
                result.result;

            setAPSRValues(
                result.carry_out,
                (result.result >> 31) & 1,
                result.overflow,
                result.result == 0
            );

            printf("Opcode::NEG\n");
            break;
        }

                case Opcode::ORR:
        {
            uint32_t result =
                regs[DE_latch.n] |
                regs[DE_latch.m];

            regs[DE_latch.destination] =
                result;

            setAPSRValues(
                xpsr.C(),
                (result >> 31) & 1,
                xpsr.V(),
                result == 0
            );

            printf("Opcode::ORR\n");
            break;
        }

        case Opcode::ROR:
        {
            uint8_t shift_n =
                regs[DE_latch.m] & 0xFF;

            auto shifted =
                shift_c(
                    regs[DE_latch.n],
                    SRType_ROR,
                    shift_n,
                    xpsr.C()
                );

            regs[DE_latch.destination] =
                shifted.result;

            setAPSRValues(
                shifted.carry_out,
                (shifted.result >> 31) & 1,
                xpsr.V(),
                shifted.result == 0
            );

            printf("Opcode::ROR\n");
            break;
        }

        case Opcode::RSB:
        {
            // RSBS Rd, Rn, #0
            auto result =
                addWithCarry(
                    ~regs[DE_latch.n],
                    0,
                    1
                );

            regs[DE_latch.destination] =
                result.result;

            setAPSRValues(
                result.carry_out,
                (result.result >> 31) & 1,
                result.overflow,
                result.result == 0
            );

            printf("Opcode::RSB\n");
            break;
        }

        case Opcode::SBC:
        {
            auto result =
                addWithCarry(
                    regs[DE_latch.n],
                    ~regs[DE_latch.m],
                    xpsr.C()
                );

            regs[DE_latch.destination] =
                result.result;

            setAPSRValues(
                result.carry_out,
                (result.result >> 31) & 1,
                result.overflow,
                result.result == 0
            );

            printf("Opcode::SBC\n");
            break;
        }

        case Opcode::SUB_REG:
        {
            auto result =
                addWithCarry(
                    regs[DE_latch.n],
                    ~regs[DE_latch.m],
                    1
                );

            regs[DE_latch.destination] =
                result.result;

            setAPSRValues(
                result.carry_out,
                (result.result >> 31) & 1,
                result.overflow,
                result.result == 0
            );

            printf("Opcode::SUB_REG\n");
            break;
        }

        case Opcode::SUB_IMM_T1:
        case Opcode::SUB_IMM_T2:
        {
            auto result =
                addWithCarry(
                    regs[DE_latch.n],
                    ~DE_latch.imm32,
                    1
                );

            regs[DE_latch.destination] =
                result.result;

            setAPSRValues(
                result.carry_out,
                (result.result >> 31) & 1,
                result.overflow,
                result.result == 0
            );

            printf("Opcode::SUB_IMM\n");
            break;
        }

        case Opcode::TST:
        {
            uint32_t result =
                regs[DE_latch.n] &
                regs[DE_latch.m];

            setAPSRValues(
                xpsr.C(),
                (result >> 31) & 1,
                xpsr.V(),
                result == 0
            );

            printf("Opcode::TST\n");
            break;
        }

        //
        // Load / store
        //

        case Opcode::LDR_LITERAL:
        {
            uint32_t base =
                (DE_latch.pc + 4) & ~0x3;

            uint32_t address =
                base + DE_latch.imm32;

            regs[DE_latch.t] =
                read32(address);

            printf("Opcode::LDR_LITERAL\n");
            break;
        }

        case Opcode::LDR_IMM:
        {
            uint32_t address =
                regs[DE_latch.n] +
                DE_latch.imm32;

            regs[DE_latch.t] =
                read32(address);

            printf("Opcode::LDR_IMM\n");
            break;
        }

        case Opcode::LDR_IMM_T2:
        {
            // SP-relative
            uint32_t address =
                regs[13] +
                DE_latch.imm32;

            regs[DE_latch.t] =
                read32(address);

            printf("Opcode::LDR_IMM_T2\n");
            break;
        }

        case Opcode::LDR_REG:
        {
            uint32_t address =
                regs[DE_latch.n] +
                shift(
                    regs[DE_latch.m],
                    SRType_LSL,
                    0,
                    xpsr.C()
                );

            regs[DE_latch.t] =
                read32(address);

            printf("Opcode::LDR_REG\n");
            break;
        }

        case Opcode::LDRB:
        {
            uint32_t address =
                regs[DE_latch.n] +
                shift(
                    regs[DE_latch.m],
                    SRType_LSL,
                    0,
                    xpsr.C()
                );

            regs[DE_latch.t] =
                (uint32_t)read8(address);

            printf("Opcode::LDRB\n");
            break;
        }

        case Opcode::LDRB_IMM:
        {
            uint32_t address =
                regs[DE_latch.n] +
                DE_latch.imm32;

            regs[DE_latch.t] =
                (uint32_t)read8(address);

            printf("Opcode::LDRB_IMM\n");
            break;
        }

        case Opcode::LDRH:
        {
            uint32_t address =
                regs[DE_latch.n] +
                shift(
                    regs[DE_latch.m],
                    SRType_LSL,
                    0,
                    xpsr.C()
                );

            regs[DE_latch.t] =
                (uint32_t)read16(address);

            printf("Opcode::LDRH\n");
            break;
        }

        case Opcode::LDRH_IMM:
        {
            uint32_t address =
                regs[DE_latch.n] +
                DE_latch.imm32;

            regs[DE_latch.t] =
                (uint32_t)read16(address);

            printf("Opcode::LDRH_IMM\n");
            break;
        }

        case Opcode::LDRSB:
        {
            uint32_t address =
                regs[DE_latch.n] +
                shift(
                    regs[DE_latch.m],
                    SRType_LSL,
                    0,
                    xpsr.C()
                );

            uint8_t value =
                read8(address);

            regs[DE_latch.t] =
                sign_extend(value, 8);

            printf("Opcode::LDRSB\n");
            break;
        }

        case Opcode::LDRSH:
        {
            uint32_t address =
                regs[DE_latch.n] +
                shift(
                    regs[DE_latch.m],
                    SRType_LSL,
                    0,
                    xpsr.C()
                );

            uint16_t value =
                read16(address);

            regs[DE_latch.t] =
                sign_extend(value, 16);

            printf("Opcode::LDRSH\n");
            break;
        }

        case Opcode::STR_IMM:
        {
            switch (DE_latch.state)
            {
                case Execute::ALU:
                {
                    uint32_t address =
                        regs[DE_latch.n] +
                        DE_latch.imm32;

                    DE_latch.write_address =
                        address;

                    DE_latch.state =
                        Execute::MEMORY;

                    stall = true;

                    printf("STR_IMM ADDRESS\n");
                    break;
                }

                case Execute::MEMORY:
                {
                    write32(
                        DE_latch.write_address,
                        regs[DE_latch.t]
                    );

                    stall = false;

                    printf("STR_IMM MEMORY\n");
                    break;
                }
            }

            printf("Opcode::STR_IMM\n");
            break;
        }

        case Opcode::STR_IMM_T2:
        {
            uint32_t address =
                regs[13] +
                DE_latch.imm32;

            write32(
                address,
                regs[DE_latch.t]
            );

            printf("Opcode::STR_IMM_T2\n");
            break;
        }

        case Opcode::STR_REG:
        {
            uint32_t address =
                regs[DE_latch.n] +
                shift(
                    regs[DE_latch.m],
                    SRType_LSL,
                    0,
                    xpsr.C()
                );

            write32(
                address,
                regs[DE_latch.t]
            );

            printf("Opcode::STR_REG\n");
            break;
        }

        case Opcode::STRB_REG:
        {
            uint32_t address =
                regs[DE_latch.n] +
                shift(
                    regs[DE_latch.m],
                    SRType_LSL,
                    0,
                    xpsr.C()
                );

            write8(
                address,
                regs[DE_latch.t] & 0xFF
            );

            printf("Opcode::STRB_REG\n");
            break;
        }

        case Opcode::STRB_IMM:
        {
            uint32_t address = regs[DE_latch.n] + DE_latch.imm32;
            printf("STRB_IMM: %x\n", address);

            write8(address, regs[DE_latch.t] & 0xFF);

            printf("Opcode::STRB_IMM\n");
            break;
        }

        case Opcode::STRH_REG:
        {
            uint32_t address =
                regs[DE_latch.n] +
                shift(
                    regs[DE_latch.m],
                    SRType_LSL,
                    0,
                    xpsr.C()
                );

            write16(
                address,
                regs[DE_latch.t] & 0xFFFF
            );

            printf("Opcode::STRH_REG\n");
            break;
        }

        case Opcode::STRH_IMM:
        {
            uint32_t address =
                regs[DE_latch.n] +
                DE_latch.imm32;

            write16(
                address,
                regs[DE_latch.t] & 0xFFFF
            );

            printf("Opcode::STRH_IMM\n");
            break;
        }

                //
        // Multiple register transfer
        //
        case Opcode::LDMIA:
        {
            uint32_t address =
                regs[DE_latch.n];

            for (uint8_t i = 0; i < 8; i++)
            {
                if (DE_latch.register_list &
                    (1u << i))
                {
                    regs[i] =
                        read32(address);

                    cycle++;

                    address += 4;

                    handleAsyncrnousExceptions();
                    handleSyncrnousExceptions();
                }
            }

            if (DE_latch.wback)
            {
                regs[DE_latch.n] +=
                    4 *
                    DE_latch.register_list_count;
            }

            printf("Opcode::LDMIA\n");
            break;
        }

        case Opcode::STMIA:
        {
            uint32_t address =
                regs[DE_latch.n];

            for (uint8_t i = 0; i < 8; i++)
            {
                if (DE_latch.register_list &
                    (1u << i))
                {
                    write32(
                        address,
                        regs[i]
                    );

                    cycle++;

                    address += 4;

                    handleAsyncrnousExceptions();
                    handleSyncrnousExceptions();
                }
            }

            if (DE_latch.wback)
            {
                regs[DE_latch.n] +=
                    4 *
                    DE_latch.register_list_count;
            }

            printf("Opcode::STMIA\n");
            break;
        }

        case Opcode::PUSH:
        {
            printf("Opcode::PUSH\n");
            
            switch(DE_latch.popState)
            {
                case PopPushState::SETUP:
                {
                    DE_latch.pop_push_address = regs[13] - (4 * DE_latch.register_list_count);
                    regs[13] -= (4 *DE_latch.register_list_count);
                    printf("Address: %x\n", regs[13]);
                    DE_latch.popState = PopPushState::TRANSFER;
                    stall = true;
                    break;
                }

               case PopPushState::TRANSFER:
                {
                    if (DE_latch.pop_push_cycles > 0)
                    {
                        for (int i = DE_latch.pop_push_iteration; i <= 14; i++)
                        {
                            if (DE_latch.register_list & (1u << i))
                            {
                                printf("push r%d=%x -> %x\n",
                                    i,
                                    regs[i],
                                    DE_latch.pop_push_address);

                                write32(DE_latch.pop_push_address, regs[i]);

                                DE_latch.pop_push_address += 4;
                                DE_latch.pop_push_cycles--;

                                if ( DE_latch.pop_push_cycles == 0)
                                {
                                    stall = false;
                                    return;
                                }

                                DE_latch.pop_push_iteration = i + 1;

                                stall = true;
                                return; // ONE transfer this cycle
                            }
                        }
                    }

                    stall = false;
                   // DE_latch.popState = PopPushState::SETUP;
                    break;
                }        
            }

            break;
        }

case Opcode::POP:
{
    printf("Opcode::POP\n");

    uint32_t address = regs[13];

    printf("ADdress: %x\n", address);
    print_mem();

    //
    // Restore low registers r0-r7
    //
    for (uint8_t i = 0; i < 8; i++)
    {
        if (DE_latch.register_list & (1u << i))
        {
            uint32_t value = read32(address);

            printf("POP r%d <= %08x from %08x\n",
                   i,
                   value,
                   address);

            regs[i] = value;

            cycle++;
            address += 4;
        }
    }

    //
    // Restore PC (bit P/M == 1)
    //
    if (DE_latch.push_pop_M)
    {
        uint32_t value = read32(address);

        printf("POP pc raw=%08x from %08x\n",
               value,
               address);

        this->branch_pc = BXWritePC2(value);

        printf("branch pc=%08x\n",
               this->branch_pc);

        cycle++;
        address += 4;

        branch_taken = true;
        flush = true;
    }

    //
    // Architecturally SP becomes address after pops
    //
    regs[13] = address;

    printf("SP after POP = %08x\n",
           regs[13]);

    break;
}

        //
        // System register access
        //
        case Opcode::MRS:
        {
            printf("Opcode::MRS\n");
            break;
        }

        case Opcode::MSR:
        {
            printf("Opcode::MSR\n");
            break;
        }

        //
        // Exception / system
        //
        case Opcode::BKPT:
        {
            printf("Opcode::BKPT\n");
            break;
        }

        case Opcode::SVC:
        {
            printf("Opcode::SVC\n");
            break;
        }

        //
        // Barriers
        //
        case Opcode::DMB:
        case Opcode::DSB:
        case Opcode::ISB:
        {
            // No-op on simple emulator
            printf("Barrier\n");
            break;
        }

        //
        // Misc / hints
        //
        case Opcode::CPS:
        {
            printf("Opcode::CPS\n");
            break;
        }

        case Opcode::NOP:
        {
            printf("Opcode::NOP\n");
            break;
        }

        case Opcode::SEV:
        {
            printf("Opcode::SEV\n");
            break;
        }

        case Opcode::WFE:
        {
            printf("Opcode::WFE\n");
            break;
        }

        case Opcode::WFI:
        {
            printf("Opcode::WFI\n");
            break;
        }

        case Opcode::YIELD:
        {
            printf("Opcode::YIELD\n");
            break;
        }

        //
        // Extend / byte manipulation
        //
        case Opcode::REV:
        {
            uint32_t x =
                regs[DE_latch.m];

            regs[DE_latch.destination] =
                ((x & 0xFF000000) >> 24) |
                ((x & 0x00FF0000) >> 8)  |
                ((x & 0x0000FF00) << 8)  |
                ((x & 0x000000FF) << 24);

            printf("Opcode::REV\n");
            break;
        }

        case Opcode::REV16:
        {
            uint32_t x =
                regs[DE_latch.m];

            regs[DE_latch.destination] =
                ((x & 0x00FF00FF) << 8) |
                ((x & 0xFF00FF00) >> 8);

            printf("Opcode::REV16\n");
            break;
        }

        case Opcode::REVSH:
        {
            uint16_t value =
                regs[DE_latch.m] &
                0xFFFF;

            uint16_t swapped =
                ((value & 0x00FF) << 8) |
                ((value & 0xFF00) >> 8);

            regs[DE_latch.destination] =
                sign_extend(
                    swapped,
                    16
                );

            printf("Opcode::REVSH\n");
            break;
        }

        case Opcode::SXTB:
        {
            regs[DE_latch.destination] =
                sign_extend(
                    regs[DE_latch.m] &
                    0xFF,
                    8
                );

            printf("Opcode::SXTB\n");
            break;
        }

        case Opcode::SXTH:
        {
            regs[DE_latch.destination] =
                sign_extend(
                    regs[DE_latch.m] &
                    0xFFFF,
                    16
                );

            printf("Opcode::SXTH\n");
            break;
        }

        case Opcode::UXTB:
        {
            regs[DE_latch.destination] =
                regs[DE_latch.m] &
                0xFF;

            printf("Opcode::UXTB\n");
            break;
        }

        case Opcode::UXTH:
        {
            regs[DE_latch.destination] =
                regs[DE_latch.m] &
                0xFFFF;

            printf("Opcode::UXTH\n");
            break;
        }

        default:
        {
            printf("Opcode::UNKNOWN\n");
            break;
        }
    }
}




        void  Cpu::test()
        {   
            FD_latch.instruction = 0;

            // set whatever values idagf
            // set pc from elf 
            // uint32_t pc = 0x100;
            cycle = 0;
            // this->fetch_pc = 0;
            this->branch_taken = false;
            this->pipeline.FD_latch.stall = false;
            this->pipeline.DE_latch.valid = false;
            this->pipeline.FD_latch.valid = false;
            this->flush = false;
            this->pipeline.DE_latch.state = Execute::ALU;
            for(uint16_t i = 0; i < 11; i++)
            {
                Pipeline next = {};

                bool instructionRetired = stage_execute(next);

                printf("Opcode::B, next branch pc: %d\n", next.DE_latch.branch_pc);
                stage_decode(next);
                stage_fetch(flash.data(), next);
                commit(next);

                cycle++;
                printf("Cycles: %d\n", cycle);
                            // handle exception at instruction bonudary
                if (instructionRetired)
                {
                    std::cout << "instruction retired\n";
                }
                print_mem();
                print_state(stdout);
            }

        }

        bool  Cpu::stage_execute(Pipeline& next)
        {
            if (!pipeline.DE_latch.valid)
            {
                return false;
            }

            printf("Opcode: %d\n", pipeline.DE_latch.opcode);
            fprintf(stderr, "pc: %d", pipeline.DE_latch.pc);
            execute_final(pipeline.DE_latch, next.DE_latch);

            printf("[EXECUTE] PC: %u\n", pipeline.DE_latch.pc);  

            return true;
        }

        void  Cpu::stage_decode(Pipeline& next)
        {
            if (pipeline.FD_latch.stall)
            {
                return;
            }

            if (!pipeline.FD_latch.valid)
            {
                return;
            }

            uint32_t instruction = pipeline.FD_latch.instruction;
            std::unordered_map<uint8_t, uint32_t> registers_read;
            decode_final(next, instruction, registers_read);
            next.DE_latch.valid = true;
            next.DE_latch.state = Execute::ALU;
            next.DE_latch.registers_read = registers_read;
            next.DE_latch.pc = pipeline.FD_latch.pc;
            printf("[DECODE] PC: %u\n", pipeline.FD_latch.pc);
        }

        void Cpu::stage_fetch(uint8_t* flash, Pipeline& next)
        {
            if (pipeline.FD_latch.stall)
            {
                return;
            }

            

            uint32_t instruction =
                flash[fetch_pc] |
                (flash[fetch_pc + 1] << 8) |
                (flash[fetch_pc + 2] << 16) |
                (flash[fetch_pc + 3] << 24);

            printf("HEX INSTRUCTION: %x\n", instruction);  

            next.FD_latch.valid = true;
            next.FD_latch.instruction = instruction;
            next.FD_latch.pc = fetch_pc;

            printf("[FETCH] PC: %u\n", fetch_pc);
        }

uint32_t  Cpu::pc_adder(Pipeline & next)
{
    if (branch_taken)
    {
        printf("BRANCH TAKEN!!!!!!!!!!!!!!!!!!!, %d\n", this->branch_pc);

        branch_taken = false;
        return this->branch_pc;
    }

    if (is32bitInstruction(next.FD_latch.instruction))
    {
        printf("Incrementing by 4\n");
        return this->fetch_pc + 4;
    }
    else 
    {
        return this->fetch_pc + 2;
    }
}

void  Cpu::commit(Pipeline & next)
{
    if (stall)
    {
        return;
    }
    else
    {
        if (flush)
        {
            next.FD_latch.valid = false;
            next.DE_latch.valid = false;
            flush = false;
        }

        printf("normal execution path\n");
        
        this->fetch_pc = pc_adder(next);
        printf("next.FD_latch.valid: %d\n", next.FD_latch.valid);
        printf("next.DE_latch.valid: %d\n", next.DE_latch.valid);
        this->pipeline = next;
    }
}

void Cpu::handle_32bit_instruction(uint32_t instruction, DecodeExecuteLatch & decode)
{
    uint8_t format_id = (instruction >> 11) & 0x1F;  
    std::cerr << "Thumb1Instruction:" << std::bitset<32>(instruction) << std::endl;

    switch (format_id)
    {
        case 0b11101:
        {
            break;
        }

        // BL
        case 0b11110:
        {
            // 1111 1000 0001 0000       1111 0000 0000 0000
            uint8_t op1 = (instruction >> 20) & 0x7F;
            uint8_t op2 = (instruction >> 12) & 0b111;

            uint16_t first  = instruction & 0xFFFF;
            uint16_t second = instruction >> 16;

            // MSR
            if ((op1 & 0b1111110) == 0b0111000 && ((op2 & 0b001) | (op2 & 0b100)) == 0b000)
            {
                uint8_t n = second & 0x7;
                uint8_t sysm = first & 0xFF;

                // decoded.decodedThumb2Instruction._MSRInstruction = MSR(n, sysm);
                // decoded.thumb2Class = BRANCH_MISC;
                // decoded.branch_misc_type = MSR_Instruction;
            
            }
            // ISB, DMS, DMB
            else if ((op1 & 0b1111111) == 0b0111011 && ((op2 & 0b001) | (op2 & 0b100)) == 0b000)
            {
                uint8_t op = (first >> 4) & 0xF;
                uint8_t option = first & 0xF;
                uint8_t second_bits_0_4 = second & 0xF;
                uint8_t first_bits_11_8 = (first >> 8) & 0xF;

                if (second_bits_0_4 != 0xF || first_bits_11_8 != 0xF)
                {
                    // Should not happen;
                    // return Decoded { };
                }

                switch (op)
                {
                    // case 0b0100:
                    //     decoded.decodedThumb2Instruction._DSBInstruction = DSB(option);
                    //     decoded.thumb2Class = BRANCH_MISC;
                    //     decoded.branch_misc_type = BL;

                    //     break;

                    // case 0b0101:
                    //     decoded.decodedThumb2Instruction._DMBInstruction = DMB(option);
                    //     decoded.thumb2Class = BRANCH_MISC;
                    //     decoded.branch_misc_type = BL;
                    //     break;

                    // case 0b0110:
                    //     decoded.decodedThumb2Instruction._ISBInstruction = ISB(option);
                    //     break;

                    // default:
                    //     break;
                }
            }
            // MRS
            else if ((op1 & 0b1111110) == 0b0111110 &&  ((op2 & 0b001) | (op2 & 0b100)) == 0b000)
            {
                // decoded.decodedThumb2Instruction._MRSInstruction = MRS(second & 0x7, first & 0xFF);
                // decoded.thumb2Class = BRANCH_MISC;
                // decoded.branch_misc_type = MRS_Instruction;
                                    // uint8_t d = second & 0x7;
                // uint8_t sysm = first & 0xFF;         
            }
            else if ((op1 & 0b1111111) == 0b1111111 && op2 == 0b010)
            {
                // undefined
            }
            else if (((op2 & 0b001) | (op2 & 0b100)) == 0b101)
            {
                uint16_t first  =
                    instruction & 0xFFFF;

                uint16_t second =
                    instruction >> 16;

                // first halfword
                uint8_t S =
                    (first >> 10) & 1;

                uint16_t imm10 =
                    first & 0x03FF;

                // second halfword
                uint8_t J1 =
                    (second >> 13) & 1;

                uint8_t J2 =
                    (second >> 11) & 1;

                uint16_t imm11 =
                    second & 0x07FF;

                // reconstruct I1/I2
                uint8_t I1 =
                    !(J1 ^ S);

                uint8_t I2 =
                    !(J2 ^ S);

                // S:I1:I2:imm10:imm11:0
                uint32_t imm25 =
                    (S << 24) |
                    (I1 << 23) |
                    (I2 << 22) |
                    (imm10 << 12) |
                    (imm11 << 1);

                int32_t imm32 =
                    sign_extend(
                        imm25,
                        25
                    );

                decode.imm32 = imm32;
                decode.opcode = Opcode::BL;
                // decoded.decodedThumb2Instruction._BranchLinkInstruction = BranchLinkInstruction(instruction);
                // decoded.thumb2Class = BRANCH_MISC;
                // decoded.branch_misc_type = BL;
            }
            break;
        }
        case 0b11111:
        {
            break;
        }
    }
}

Opcode Cpu::decode_alu(uint8_t opcode)
{
    
    Opcode val;
    switch (opcode)
    {
        case 0x0: // AND
        {
            val = Opcode::AND;
            break;
        }

        case 0x1: // EOR
        {
            val = Opcode::EOR;
            break;
        }

        case 0x2: // LSL (register)
        {
            val = Opcode::LSL_REG;
            break;
        }

        case 0x3: // LSR (register)
        {
        
            val = Opcode::LSR_REG;
            break;
        }

        case 0x4: // ASR (register)
        {
            val = Opcode::ASR_REG;
            break;
        }

        case 0x5: // ADC
        {
            val = Opcode::ADC;
            break;
        }
        case 0x6: // SBC
        {
            val = Opcode::SBC;
            break;
        }

        case 0x7: // ROR
        {
            val = Opcode::ROR;
            break;
        }

        case 0x8: // TST (no write)
        {
            val = Opcode::TST;
            break;
        }

        case 0x9: // NEG
        {
            val = Opcode::NEG;
            break;
        }

        case 0xA: // CMP (no write)
        {
            val = Opcode::CMP_REG;
            break;
        }

        case 0xB: // CMN (no write)
        {
            val = Opcode::CMN;
            break;
        }

        case 0xC: // ORR
        {
            val = Opcode::ORR;
            break;
        }
        case 0xD: // MUL
        {
            val = Opcode::MUL;
            break;
        }

        case 0xE: // BIC
        {
            val = Opcode::BIC;
            break;
        }

        case 0xF: // MVN
        {
            val = Opcode::MVN;
            break;
        }
    }
                
    return val;
}

Opcode Cpu::decode_load_store_reg(uint8_t opcode)
{
    switch (opcode) 
    {
        case 0b000: // STR
        {      
            return Opcode::STR_REG;
        }
        case 0b001: // STRH
        {
            return Opcode::STRH_REG;
        }
        case 0b010: // STRB
        {   
            return Opcode::STRB_REG;
        }
        case 0b011: // LDRSB
        {
            return Opcode::LDRSB;
        }
        case 0b100: // LDR
        {
            return Opcode::LDR_REG;
        }

        case 0b101: // LDRH
        {
            return Opcode::LDRH;
        }
        case 0b110: // LDRB
        {
            return Opcode::LDRB;
        }
        case 0b111: // LDRSH
        {
            return Opcode::LDRSH;
        }
        default:
            return Opcode::INVALID;
    }
}

Opcode Cpu::decode_special(uint8_t opcode, uint8_t d, uint8_t m, bool H1)
{
    switch (opcode)
    {
        case 0b00: // ADD (high register)
        {
            fprintf(stderr, "ADD (Register)\n");

            // SP special case
            if (d == 13 || m == 13)
            {
                std::cerr << "SEE ADD (SP plus register)\n";

                return Opcode::ADD_SP_T1;
            }
            break;
        }

        case 0b01: // CMP (high register)
        {
            fprintf(stderr,
                    "CMP (Register)\n");

            uint8_t n = d;

            // both low regs = unpredictable
            if (n < 8 && m < 8)
            {
                std::cerr
                    << "Unpredictable\n";
                return Opcode::INVALID;
            }

            // PC forbidden
            if (n == 15 || m == 15)
            {
                std::cerr << "Unpredictable\n";
                return Opcode::INVALID;
            }

            break;
        }

        case 0b10: // MOV (register)
        {
            fprintf(stderr,"MOV (Register) Spec\n");

            return Opcode::MOV_REG;
            break;
        }

        case 0b11: // BX / BLX
        {
            if (H1)
            {
                return Opcode::BX;

            }

            return Opcode::BLX;

            break;
        }
    }
}

Opcode Cpu::decode_load_store_half(bool opcode)
{
    if (opcode) 
    {
        return Opcode::LDRH_IMM;
    } 
    else 
    {
        return Opcode::STRH_IMM;
    }
}

Opcode Cpu::decode_load_store_imm(uint8_t opcode)
{
    switch (opcode) 
    {
        case 0b00: // STR (word)
        {
            
            return Opcode::STR_IMM;
        }

        case 0b01: // LDR (word)
        {
            return Opcode::LDR_IMM;

        }

        case 0b10: // STRB
        {
            return Opcode::STRB_IMM;

            break;
        }

        case 0b11: // LDRB
        {                    
            return Opcode::LDRB_IMM;

            // break;
        }

        default:
            return Opcode::INVALID;
    }
}




Opcode Cpu::decode_misc(uint16_t instr, DecodeExecuteLatch & DE_latch, std::unordered_map<uint8_t, uint32_t> & registers_read)
{
    if ((instr & 0xFFFF) == 0b1011111101000000) 
    {
        fprintf(stderr, "SEV\n");

        return Opcode::SEV;
    }

    if ((instr & 0xFFFF) == 0b1011111100000000) 
    {
        fprintf(stderr, "NOP\n");
        return Opcode::NOP;
    }

    if ((instr & 0xFFFF) == 0b1101111100000000) 
    {
        fprintf(stderr, "SVC Thumb1Instruction");
        return Opcode::SVC;
    }

    if ((instr & 0b1111111111100000) == 0b1011011001100000) 
    {
        fprintf(stderr, "CPS\n");
        return Opcode::CPS;
    }

    // ---- ADD / SUB SP ----
    if ((instr & 0b1111111100000000) == 0b1011000000000000) 
    {
        uint8_t imm7 = instr & 0x7F;
        DE_latch.imm32 = ((uint32_t) imm7 << 2);
        bool bits_7 = (instr >> 7) & 0x1;
        uint8_t opcode = (instr >> 8) & 0xF;
        DE_latch.destination = 13;
        if (bits_7)
        {
            return Opcode::SUB_SP_T1;
        }
        else
        {
            return Opcode::ADD_SP_T2;
        }
    }

    // // ---- PUSH / POP ----
    if ((instr & 0b1111000000000000) == 0b1011000000000000) 
    {
        uint8_t register_list = instr & 0xFF;
        bool M = (instr >> 8) & 0x1;
        uint8_t bits_10_9 = (instr >> 9) & 0x3;
        uint8_t op = (instr >> 11) & 0x1;

        if (op) 
        {
            DE_latch.register_list = register_list | (M << 15);
            DE_latch.registers_read.emplace(13, regs[13]);
            DE_latch.register_list_count = std::bitset<16>(DE_latch.register_list).count();
            DE_latch.pop_push_cycles = std::bitset<16>(DE_latch.register_list).count();
            DE_latch.push_pop_M = M;

            return Opcode::POP;
        } 
        else 
        {   
            printf("DOES THIS EVEN TGET ESXECUTED\n");
            
            DE_latch.register_list = register_list | (M << 14);
            // while(1);
            
            printf("PUSH Register list: %d\n", DE_latch.register_list);
            printf("BOOL M: %d\n", M);
            DE_latch.registers_read.emplace(13, regs[13]);
            DE_latch.register_list_count = std::bitset<16>(DE_latch.register_list).count();
            DE_latch.pop_push_cycles = std::bitset<16>(DE_latch.register_list).count();
            DE_latch.popState = PopPushState::SETUP;
            return Opcode::PUSH;
        }
    }

    // ---- EXTEND (UXTB/SXTB/UXTH/SXTH) ----
    if ((instr & 0b1111110000000000) == 0b1011001000000000) 
    {
        uint8_t d = instr & 0x7;
        uint8_t m = (instr >> 3) & 0x7;
        uint8_t op = (instr >> 6) & 0x3;
        uint8_t bits_11_8 = (instr >> 8) & 0xF;

        switch (op) 
        {
            case 0:
            {
                return Opcode::SXTH;
                // SXTH
            }
            case 1:
            {
                return Opcode::SXTB;
            }
            
            case 2:               
            {
                return Opcode::UXTH;
            }
            case 3:                     
            {
                return Opcode::UXTB;
            }
        }
    }

    // REV family
    if ((instr & 0b1111110000000000) == 0b1011101000000000) 
    {
        return Opcode::REV;
    }
}


void Cpu::decode_final(
    Pipeline& next,
    uint32_t instruction,
    std::unordered_map<uint8_t, uint32_t>& registers_read)
{
    DecodeExecuteLatch& decode = next.DE_latch;

    // Reset latch
    decode = {};

    //
    // Thumb-2
    //
    if (is32bitInstruction(instruction))
    {
        handle_32bit_instruction(instruction, next.DE_latch);
        return;
    }

    auto cls = classify(instruction);

    std::cout << "Thumb1Instruction: "
              << std::bitset<16>((uint16_t)instruction)
              << std::endl;

    std::cout << "Class: "
              << static_cast<int>(cls)
              << std::endl;

    switch (cls)
    {
        //
        // Shift immediate
        // LSL/LSR/ASR (imm)
        //
        case InstrClass::SHIFT_IMM:
        {
            uint8_t d = instruction & 0x7;
            uint8_t m = (instruction >> 3) & 0x7;
            uint8_t imm5 = (instruction >> 6) & 0x1F;
            uint8_t op = (instruction >> 11) & 0x3;

            auto result = decodeImmShift(op, imm5);

            decode.destination = d;
            decode.m = m;

            decode.shift_t = result.type;
            decode.shift_n = result.n;

            registers_read.emplace(m, regs[m]);

            switch (op)
            {
                case 0b00:
                    decode.opcode = Opcode::LSL_IMM;
                    break;

                case 0b01:
                    decode.opcode = Opcode::LSR_IMM;
                    break;

                case 0b10:
                    decode.opcode = Opcode::ASR_IMM;
                    break;

                default:
                    decode.opcode = Opcode::INVALID;
                    break;
            }

            break;
        }

        //
        // ADD/SUB (register/immediate)
        //
        case InstrClass::ADD_SUB:
        {
            uint8_t d = instruction & 0x7;
            uint8_t n = (instruction >> 3) & 0x7;

            bool immediate = (instruction >> 10) & 1;
            bool sub = (instruction >> 9) & 1;

            decode.destination = d;
            decode.n = n;

            registers_read.emplace(n, regs[n]);

            if (immediate)
            {
                decode.imm32 = (instruction >> 6) & 0x7;

                decode.opcode =
                    sub ? Opcode::SUB_IMM_T1
                        : Opcode::ADD_IMM_T1;
            }
            else
            {
                uint8_t m = (instruction >> 6) & 0x7;

                decode.m = m;

                registers_read.emplace(m, regs[m]);

                // You currently only have immediate opcodes
                decode.opcode =
                    sub ? Opcode::SUB_IMM_T1
                        : Opcode::ADD_IMM_T1;
            }

            break;
        }

        //
        // MOV/CMP/ADD/SUB immediate
        //
        case InstrClass::MOV_CMP_ADD_SUB:
        {
            uint8_t op = (instruction >> 11) & 0x3;
            uint8_t d = (instruction >> 8) & 0x7;
            uint8_t imm8 = instruction & 0xFF;

            decode.destination = d;
            decode.n = d;
            decode.imm32 = imm8;

            switch (op)
            {
                case 0b00: // MOV
                {
                    decode.opcode = Opcode::MOV_IMM;
                    break;
                }

                case 0b01: // CMP
                {
                    registers_read.emplace(d, regs[d]);
                    decode.opcode = Opcode::CMP_IMM;
                    break;
                }

                case 0b10: // ADD
                {
                    registers_read.emplace(d, regs[d]);
                    decode.opcode = Opcode::ADD_IMM_T2;
                    break;
                }

                case 0b11: // SUB
                {
                    registers_read.emplace(d, regs[d]);
                    decode.opcode = Opcode::SUB_IMM_T2;
                    break;
                }
            }

            break;
        }

        //
        // ALU subgroup
        //
        case InstrClass::ALU:
        {
            uint8_t n = instruction & 0x7;
            uint8_t m = (instruction >> 3) & 0x7;
            uint8_t opcode = (instruction >> 6) & 0xF;

            decode.n = n;
            decode.m = m;
            decode.destination = n;

            registers_read.emplace(n, regs[n]);
            registers_read.emplace(m, regs[m]);

            decode.opcode = decode_alu(opcode);

            break;
        }

        //
        // High register ops / BX / BLX
        //
        case InstrClass::HI_REG:
        {
            uint8_t op = (instruction >> 8) & 0b11;

            bool H1 = (instruction >> 7) & 1;
            bool H2 = (instruction >> 6) & 1;

            uint8_t m =
                ((instruction >> 3) & 0x7) |
                (H2 << 3);

            uint8_t d =
                (instruction & 0x7) |
                (H1 << 3);

            decode.destination = d;
            decode.n = d;
            decode.m = m;

            registers_read.emplace(m, regs[m]);
            registers_read.emplace(d, regs[d]);

            decode.opcode = decode_special(op, d, m, H1);

            break;
        }

        //
        // LDR literal
        //
        case InstrClass::LDR_LITERAL:
        {
            decode.t = (instruction >> 8) & 0x7;
            decode.imm32 =
                (instruction & 0xFF) << 2;

            decode.opcode =
                Opcode::LDR_LITERAL;

            break;
        }

        //
        // Register load/store subgroup
        //
        case InstrClass::LOAD_STORE_REG:
        {
            uint8_t t = instruction & 0x7;
            uint8_t n = (instruction >> 3) & 0x7;
            uint8_t m = (instruction >> 6) & 0x7;
            uint8_t op = (instruction >> 9) & 0x7;

            decode.t = t;
            decode.n = n;
            decode.m = m;

            decode.shift_t = SRType_LSL;
            decode.shift_n = 0;

            registers_read.emplace(n, regs[n]);
            registers_read.emplace(m, regs[m]);
            registers_read.emplace(t, regs[t]);

            decode.opcode =
                decode_load_store_reg(op);

            break;
        }

        //
        // Immediate load/store
        //
        case InstrClass::LOAD_STORE_IMM:
        {
            uint8_t t = instruction & 0x7;
            uint8_t n = (instruction >> 3) & 0x7;
            uint8_t imm5 = (instruction >> 6) & 0x1F;
            uint8_t op = (instruction >> 11) & 0x3;
            decode.state = Execute::ALU; 
            decode.t = t;
            decode.n = n;

            // STR/LDR word scale by 4
            decode.imm32 =(op <= 0b01) ? (imm5 << 2): imm5;

            registers_read.emplace(t, regs[t]);
            registers_read.emplace(n, regs[n]);

            decode.opcode = decode_load_store_imm(op);

            break;
        }

        //
        // Halfword load/store
        //
        case InstrClass::LOAD_STORE_HALF:
        {
            uint8_t t = instruction & 0x7;
            uint8_t n = (instruction >> 3) & 0x7;
            uint8_t imm5 =
                (instruction >> 6) & 0x1F;

            bool load =
                (instruction >> 11) & 1;

            decode.t = t;
            decode.n = n;

            // halfword => <<1
            decode.imm32 = imm5 << 1;

            registers_read.emplace(t, regs[t]);
            registers_read.emplace(n, regs[n]);

            decode.opcode =
                decode_load_store_half(load);

            break;
        }

        //
        // SP-relative load/store
        //
        case InstrClass::SP_REL:
        {
            bool load =
                (instruction >> 11) & 1;

            decode.t =
                (instruction >> 8) & 0x7;

            decode.imm32 =
                (instruction & 0xFF) << 2;

            registers_read.emplace(13, regs[13]);

            decode.opcode =
                load
                    ? Opcode::LDR_IMM_T2
                    : Opcode::STR_IMM_T2;

            break;
        }

        //
        // ADR / ADD SP
        //
        case InstrClass::ADDR:
        {
            bool addSP =
                (instruction >> 11) & 1;

            decode.destination =
                (instruction >> 8) & 0x7;

            decode.imm32 =
                (instruction & 0xFF) << 2;

            decode.opcode =
                addSP
                    ? Opcode::ADD_SP_T1
                    : Opcode::ADR;

            break;
        }

        //
        // Misc subgroup
        //
        case InstrClass::MISC:
        {
            decode.opcode = decode_misc(instruction, decode, registers_read);
            break;
        }

        //
        // LDM/STM
        //
        case InstrClass::MULTIPLE:
        {
            bool load =
                (instruction >> 11) & 1;

            uint8_t n =
                (instruction >> 8) & 0x7;

            decode.n = n;

            decode.register_list =
                instruction & 0xFF;

            decode.register_list_count =
                std::bitset<8>(
                    decode.register_list)
                    .count();

            decode.wback =
                load
                    ? ((decode.register_list &
                        (1 << n)) == 0)
                    : true;

            registers_read.emplace(
                n,
                regs[n]);

            decode.opcode =
                load
                    ? Opcode::LDMIA
                    : Opcode::STMIA;

            if (decode.register_list_count == 0)
            {
                decode.opcode =
                    Opcode::INVALID;
            }

            break;
        }

        //
        // Conditional branch
        //
        case InstrClass::COND_BRANCH:
        {
            decode.cond =
                (instruction >> 8) & 0xF;

            decode.imm32 =
                sign_extend(
                    (instruction & 0xFF)
                    << 1,
                    9);

            decode.opcode =
                Opcode::B_COND;

            break;
        }

        //
        // Unconditional branch
        //
        case InstrClass::UNCOND_BRANCH:
        {
            decode.opcode = Opcode::B;

            decode.imm32 =
                sign_extend(
                    (instruction & 0x7FF)
                    << 1,
                    12);

            break;
        }

        default:
        {
            std::cerr
                << "UNKNOWN\n";

            decode.opcode =
                Opcode::INVALID;
            break;
        }
    }
}

// void  Cpu::decode_final(Pipeline & next, uint32_t instruction, std::unordered_map<uint8_t, uint32_t> & registers_read)
// {
//     if (is32bitInstruction(instruction))
//     {
//         handle_32bit_instruction(instruction);
//     }
//     else
//     {
//         auto cls = classify(instruction);
//         std::cout << "Thumb1Instruction:" << std::bitset<16>((uint16_t)instruction) << std::endl;
//         std::cout << "Class: " << static_cast<int>(cls) << std::endl;

//         DecodeExecuteLatch & decode = next.DE_latch;

//         switch (cls)
//         {
//             case InstrClass::SHIFT_IMM:
//             {
//                 uint8_t d = instruction & 0x7;
//                 uint8_t m = (instruction >> 3) & 0x7;
//                 uint8_t imm5 = (instruction >> 6) & 0x1F;
//                 uint8_t type = (instruction >> 11) & 0x3;

//                 auto result = decodeImmShift(type, imm5);

//                 decode.destination = d;
//                 decode.m = m;

//                 registers_read.emplace(m, regs[m]);

//                 decode.shift_t = result.type;
//                 decode.shift_n = result.n;

//                 switch (type)
//                 {
//                     case 0:
//                         decode.opcode = Opcode::LSL_IMM;
//                         break;

//                     case 1:
//                         decode.opcode = Opcode::LSR_IMM;
//                         break;

//                     case 2:
//                         decode.opcode = Opcode::ASR_IMM;
//                         break;

//                     default:
//                         decode.opcode = Opcode::INVALID;
//                         break;
//                 }

//                 break;
//             }

//             case InstrClass::ADD_SUB:
//             {
//                 decode.destination = instruction & 0x7;

//                 uint8_t n = (instruction >> 3) & 0x7;
//                 uint8_t operand = (instruction >> 6) & 0x7;

//                 bool immediate = (instruction >> 10) & 1;
//                 bool subtract  = (instruction >> 9) & 1;

//                 decode.n = n;

//                 registers_read.emplace(n, regs[n]);

//                 if (immediate)
//                 {
//                     decode.imm32 = operand;

//                     decode.opcode =
//                         subtract ?
//                         Opcode::SUB_IMM_T1 :
//                         Opcode::ADD_IMM_T1;
//                 }
//                 else
//                 {
//                     decode.m = operand;

//                     registers_read.emplace(operand, regs[operand]);

//                     decode.opcode =
//                         subtract ?
//                         Opcode::SUB_REG :
//                         Opcode::ADD_REG;
//                 }

//                 break;
//             }

//             case InstrClass::MOV_CMP_ADD_SUB:
//             {
//                 uint8_t op = (instruction >> 11) & 0x3;
//                 uint8_t d = (instruction >> 8) & 0x7;

//                 decode.destination = d;
//                 decode.imm32 = instruction & 0xFF;

//                 switch (op)
//                 {
//                     case 0:
//                         decode.opcode = Opcode::MOV_IMM;
//                         break;

//                     case 1:
//                         decode.opcode = Opcode::CMP_IMM;
//                         registers_read.emplace(d, regs[d]);
//                         break;

//                     case 2:
//                         decode.opcode = Opcode::ADD_IMM_T2;
//                         registers_read.emplace(d, regs[d]);
//                         break;

//                     case 3:
//                         decode.opcode = Opcode::SUB_IMM_T2;
//                         registers_read.emplace(d, regs[d]);
//                         break;
//                 }

//                 break;
//             }

//             case InstrClass::ALU:
//             {
//                 uint8_t n = instruction & 0x7;
//                 uint8_t m = (instruction >> 3) & 0x7;
//                 decode.n = n;
//                 decode.destination = instruction & 0x7;
//                 registers_read.emplace(n, regs[n]);
//                 registers_read.emplace(m, regs[m]);
//                 decode.m = m;
//                 uint8_t opcode = (instruction >> 6) & 0xF;
//                 decode.opcode = decode_alu(opcode);
//                 break;
//             }

//             case InstrClass::HI_REG:
//             {
//                 uint8_t op = (instruction >> 8) & 0b11;

//                 bool H1 = (instruction >> 7) & 1;
//                 bool H2 = (instruction >> 6) & 1;

//                 // full 4-bit registers
//                 uint8_t m = ((instruction >> 3) & 0b111) | (H2 << 3);

//                 uint8_t d = (instruction & 0b111) | (H1 << 3);
//                 registers_read.emplace(m, regs[m]);
//                     decode.m = m;
//                     decode.destination = d;
//                 // registers_read.emplace(m, regs[m]);
//                 decode.opcode = decode_special(op, d, m, H1);

//                 break;
//             }

//             case InstrClass::LDR_LITERAL:
//             {
//                 decode.imm32 = (instruction & 0xFF) << 2;

//                 decode.t = (instruction >> 8) & 0x7;
//                 decode.opcode = Opcode::LDR_LITERAL;

//                 break;
//             }

//             case InstrClass::LOAD_STORE_REG:
//             {
//                 printf("LOAD_STORE_REG\n");
//                 uint8_t t = instruction & 0x7;
//                 uint8_t n = (instruction >> 3) & 0x7;
//                 uint8_t m = (instruction >> 6) & 0x7;
//                 uint8_t opcode = (instruction>> 9) & 0x7;

//                 decode.opcode = decode_load_store_reg(opcode);
//                 decode.shift_t = SRType_LSL;
//                 decode.shift_n = 0;
                
//                 registers_read.emplace(n, regs[n]);
//                 registers_read.emplace(m, regs[m]);
//                 registers_read.emplace(t, regs[t]);

//                 break;
//             }
//                 //  decoded.decodedInstruction.loadStoreRegInstruction = LoadStoreRegInstruction(instruction);
    

//             case InstrClass::LOAD_STORE_IMM:

//             printf("STORE IMM\n");
//             {
//                 uint8_t t = instruction & 0x7;
//                 uint8_t n = (instruction >> 3) & 0x7;
//                 decode.imm32 = (instruction >> 6) & 0x1F;
//                 uint8_t opcode = (instruction >> 11) & 0x3;
//                 decode.registers_read.emplace(t, regs[t]);
//                 decode.registers_read.emplace(n, regs[n]);
//                 decode.t = t;
//                 decode.n = n;
//                 decode.opcode = decode_load_store_imm(opcode);
//                 break;
//             }


//             case InstrClass::LOAD_STORE_HALF:
//             {
//                 uint8_t t = instruction & 0x7;
//                 uint8_t n = (instruction >> 3) & 0x7;
//                 uint8_t imm5 = (instruction >> 6) & 0x1F;
//                 bool opcode = (instruction >> 11) & 0x1;
                
//                 decode.imm32 = (uint32_t)(imm5 << 2);

//                 decode.registers_read.emplace(t, regs[t]);
//                 decode.registers_read.emplace(n, regs[n]);
                
//                 decode.t = t;
//                 decode.n = n;

//                 decode.opcode = decode_load_store_half(opcode);

//                 break;
//             }

//             case InstrClass::SP_REL:
//             {
//                 uint8_t imm8 = instruction & 0xFF;
//                 uint8_t t = (instruction >> 8) & 0x7;
//                 bool opcode = (instruction >> 11) & 0x1;

//                 decode.imm32 = (uint32_t)(imm8 << 2);
//                 decode.t = t;
                
//                 registers_read.emplace(t, regs[t]);
                
//                 if (opcode)
//                 {
//                     decode.opcode = Opcode::LDR_IMM_T2;
//                 }
//                 else decode.opcode = Opcode::STR_IMM_T2;

//                 break;
//             }

//             case InstrClass::ADDR:
//             {
//                 uint8_t imm8 = instruction & 0xFF;
//                 uint8_t d = (instruction >> 8) & 0x7;
//                 bool opcode = (instruction >> 11) & 0x1;

//                 decode.destination = d;
//                 decode.imm32 = ((uint32_t) imm8) << 2;

//                 if (opcode) 
//                 {
//                     decode.opcode = Opcode::ADD_SP_T1;
//                 } 
//                 else 
//                 {
//                     decode.opcode = Opcode::ADR;
//                 } 

//                 break;
//             }

//             case InstrClass::MISC:
//             {
//                     fprintf(stderr, "Handle MISC: \n");
// // std::cout << std::hex << instr << std::endl;
//                 decode.opcode = decode_misc(instruction, decode, registers_read);

//                 break;
//             }

//             case InstrClass::MULTIPLE:
//             {
//                 next.DE_latch.register_list = instruction & 0xFF;
//                 uint8_t n = (instruction >> 8) & 0x7;
//                 bool op = (instruction >> 11) & 0x1;
//                 // bool wback = true;
//                 next.DE_latch.register_list_count = std::bitset<16>(next.DE_latch.register_list).count();
                
//                 if (op)
//                 {
//                     next.DE_latch.wback = ( next.DE_latch.register_list & 0b1 << n) == 0;
//                     next.DE_latch.opcode = Opcode::LDMIA;

//                     next.DE_latch.n = n;

//                     registers_read.emplace(n, regs[n]);

//                     if (next.DE_latch.register_list_count < 1)
//                     {
//                         std::cerr << "unpredictable" << std::endl;
//                         next.DE_latch.opcode = Opcode::INVALID;
//                     }

//                 }
//                 else
//                 {
//                     next.DE_latch.wback = true;
//                     next.DE_latch.opcode = Opcode::LDMIA;

//                         registers_read.emplace(n, regs[n]);
//                     if (next.DE_latch.register_list_count < 1)
//                     {
//                         std::cerr << "unpredictable" << std::endl;
//                         next.DE_latch.opcode = Opcode::INVALID;
//                     }
//                 }

//                 break;
//             }
//             case InstrClass::COND_BRANCH:
//             {
//                 uint8_t imm8 = instruction & 0xFF;

//                 next.DE_latch.imm32 = sign_extend(imm8 << 1, 9);
//                 next.DE_latch.cond = (instruction >> 8) & 0xF;
                
//                 break;
//             }

//             case InstrClass::UNCOND_BRANCH:
//             {
//                 next.DE_latch.opcode = Opcode::B;   
//                 next.DE_latch.imm32 = sign_extend((instruction & 0x7FF) << 1, 12);

//                 printf("HELLO: %d\n", next.DE_latch.imm32);

//                 break;
//             }
//             case InstrClass::UNKNOWN:
//             default:
//                 std::cerr << "UNKNOWN\n";
//                 break;
//         }     
//     }
// }

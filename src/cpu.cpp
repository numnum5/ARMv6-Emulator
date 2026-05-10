#include "cpu.hpp"

constexpr uint32_t UART_BASE = 0x40000000;
constexpr uint32_t UART_DR   = UART_BASE + 0x0;
constexpr uint32_t UART_SR   = UART_BASE + 0x4;
Cpu::Cpu(size_t ram_size, size_t flash_size) : ram(ram_size), flash(flash_size)
{
    for(uint8_t i = 0; i < 16; i++)
    {
        this->regs[i] = 0;
    }

    this->aspr.C = 0;
    this->aspr.V = 0;
    this->aspr.N = 0;
    this->aspr.Z = 0;
}   

uint32_t Cpu::getSP(void) const
{
    return this->regs[13];
}


uint32_t Cpu::read32Flash(uint32_t address) const
{
    address = address - FLASH_BASE;
    uint32_t data = this->flash[address] |
        (this->flash[address + 1] << 8) |
        (this->flash[address + 2] << 16) |
        (this->flash[address + 3] << 24);

    return data;
} 

bool Cpu::conditionPassed(uint8_t cond) const
{
    bool result = false;

    switch(cond >> 1)
    {
    case 0b000:
        result = aspr.Z;
        break;

    case 0b001:
        result = aspr.C;
        break;

    case 0b010:
        result = aspr.N;
        break;

    case 0b011:
        result = aspr.V;
        break;

    case 0b100:
        result = aspr.C && !aspr.Z;
        break;

    case 0b101:
        result = (aspr.N == aspr.V);
        break;

    case 0b110:
        result = (aspr.N == aspr.V) && !aspr.Z;
        break;

    case 0b111:
        result = true;
        break;
    }

    // invert condition except AL
    if ((cond & 1) && cond != 0b1110)
    {
        result = !result;
    }

    return result;
}

uint32_t Cpu::read32(uint32_t address) const
{
    address = address - RAM_BASE;

    uint32_t data = this->ram[address] |
    (this->ram[address + 1] << 8) |
    (this->ram[address + 2] << 16) |
    (this->ram[address + 3] << 24);

    return data;
}




void Cpu::write32(uint32_t address, uint32_t value)
{
    if(address == UART_DR)
    {
        printf("%c", value & 0xFF);
        
        fflush(stdout);
        return;
    }

    address = address - RAM_BASE;
    this->ram[address] = value & 0xFF;
    this->ram[address + 1] = (value >> 8) & 0xFF;
    this->ram[address + 2] = (value >> 16) & 0xFF;
    this->ram[address + 3] = (value >> 24) & 0xFF;
}

void Cpu::write16(uint32_t address, uint16_t value)
{
 
    printf("write16(): address: %x\n", address);
    address = address - RAM_BASE;
    this->ram[address] = value & 0xFF;
    this->ram[address + 1] = (value >> 8) & 0xFF;
}

void Cpu::write8(uint32_t address, uint8_t value)
{
    address = address - RAM_BASE;
    this->ram[address] = value;
}

uint8_t Cpu::read8(uint32_t address) const
{
    address = address - RAM_BASE;
    uint8_t data = this->ram[address];
    
    return data;
}

uint16_t Cpu::read16(uint32_t address) const 
{
    uint32_t data = this->ram[address] | (this->ram[address + 1] << 8);

    return data;
}   

InstrClass Cpu::classify(uint16_t instr)
{
    if ((instr & 0b1111000000000000) == 0b1101000000000000) 
    {
        if ((instr & 0b0000111100000000) == 0b0000111100000000)
            return InstrClass::SVC;          // 1101 1111 xxxx xxxx
        return InstrClass::COND_BRANCH;      // 1101 xxxx xxxx xxxx
    }

    if ((instr & 0b1111100000000000) == 0b1110000000000000) 
    {
        return InstrClass::UNCOND_BRANCH;    // 11100 xxxxx xxxxx
    }

    if ((instr & 0b1111110000000000) == 0b0100000000000000) 
    {
        return InstrClass::ALU;              // 010000 xxxxxx xxxx
    }

    if ((instr & 0b1111110000000000) == 0b0100010000000000) 
    {
        return InstrClass::HI_REG;           // 010001 xxxxxx xxxx
    }

    if ((instr & 0b1111100000000000) == 0b0100100000000000) 
    {
        return InstrClass::LDR_LITERAL;      // 01001 xxxxx xxxxx
    }

    if ((instr & 0b1111000000000000) == 0b0101000000000000) 
    {
        return InstrClass::LOAD_STORE_REG;   // 0101xx xxxx xxxx
    }

    if ((instr & 0b1110000000000000) == 0b0110000000000000) {
        return InstrClass::LOAD_STORE_IMM;   // 011xx xxxx xxxx
    }

    if ((instr & 0b1111000000000000) == 0b1000000000000000) 
    {
        return InstrClass::LOAD_STORE_HALF;  // 1000x xxxx xxxx
    }

    if ((instr & 0b1111000000000000) == 0b1001000000000000) {
        return InstrClass::SP_REL;           // 1001x xxxx xxxx
    }

    if ((instr & 0b1111000000000000) == 0b1010000000000000) {
        return InstrClass::ADDR;             // 1010x xxxx xxxx
    }

    if ((instr & 0b1111000000000000) == 0b1011000000000000) {
        return InstrClass::MISC;             // 1011xxxx xxxx xxxx
    }

    if ((instr & 0b1111000000000000) == 0b1100000000000000) {
        return InstrClass::MULTIPLE;         // 1100x xxxx xxxx
    }

    // 2. Lower groups

    if ((instr & 0b1110000000000000) == 0b0000000000000000) {

        uint16_t op = (instr >> 11) & 0b11111;
        

        if (op == 0b00011)
            return InstrClass::ADD_SUB;

        if ((op & 0b11000) == 0b00000)
        {
            return InstrClass::SHIFT_IMM;
            printf("THIS IS LSL\n");
        }
    }

    if ((instr & 0b1110000000000000) == 0b0010000000000000) {
        return InstrClass::MOV_CMP_ADD_SUB;  // 001xx xxxx xxxx
    }

    return InstrClass::UNKNOWN;
}
void Cpu::handleAddSub(uint16_t instr)
{
    uint8_t I  = (instr >> 10) & 0x1;
    uint8_t Op = (instr >> 9)  & 0x1;

    uint8_t Rn = (instr >> 3) & 0x7;
    uint8_t Rs = (instr >> 6) & 0x7;
    uint8_t Rd = instr & 0x7;

    uint32_t operand2;

    // immediate or register
    if (I)
        operand2 = Rs;          // imm3
    else
        operand2 = regs[Rs];    // register

    uint32_t result;

    if (Op == 0)
    {
        // ADD
        printf("ADD\n");
        auto res = addWithCarry(regs[Rn], operand2, 0);

        result = res.result;

        aspr.N = (result >> 31) & 1;
        aspr.Z = (result == 0);
        aspr.C = res.carry_out;
        aspr.V = res.overflow;
    }
    else
    {
                printf("SUB\n");
        // SUB
        auto res = addWithCarry(regs[Rn], ~operand2, 1);

        result = res.result;

        aspr.N = (result >> 31) & 1;
        aspr.Z = (result == 0);
        aspr.C = res.carry_out;
        aspr.V = res.overflow;
    }

    regs[Rd] = result;

    regs[15] += 2;
}

void Cpu::handleSpecialInstructions(uint16_t instruction)
{
    uint8_t op = (instruction >> 8) & 0b11;
    uint8_t H1 = (instruction >> 7) & 0b1;
    uint8_t H2 = (instruction >> 6) & 0b1;

    uint8_t m = ((instruction >> 3) & 0b111) | (H2 << 3);
    uint8_t d = (instruction & 0b111) | (H1 << 3);

    // 1011 0101 1000 0000
    switch (op)
    {
        case 0b00: // ADD (high register)
        {
            printf("ADD (Register)\n");
            bool sevenBit = (instruction >> 7) & 0b1;

            uint8_t DN_rdn = (d << 1) | sevenBit;

            if (DN_rdn == 0b1101 || m == 0b1101)
            {

            }

            regs[d] = regs[d] + regs[m];


            break;
        }
        case 0b01: // CMP (high register)
        {   
            printf("CMP (Register)\n");
            bool N = (instruction >> 7) & 0b1;

            uint8_t m = ((instruction >> 3) & 0b111);
            uint8_t n = (instruction & 0b111);
            if (n < 8 && m < 8)
            {   
                std::cout << "Unpredictable\n";
                return;
            }

            if (n == 15 || m == 15)
            {
                std::cout << "Unpredictable\n";
                return;
            }

            const auto shifted = shift(this->regs[m], SRType_LSL, 0, aspr.C);
           
            const auto result = addWithCarry(this->regs[n], ~shifted, 1);

            this->aspr.C = result.carry_out;
            this->aspr.V = result.overflow;
            this->aspr.Z = result.result == 0;
            this->aspr.N = (result.result >> 31) & 0b1;
            
            // updateFlagsSub(regs[rd], regs[rm], result);
            break;
        }

        case 0b10: // MOV (register)
        {
            std::cout << "MOV (register)\n";
            bool N = (instruction >> 7) & 0b1;

            uint8_t m = ((instruction >> 3) & 0b1111);
            uint8_t d = (instruction & 0b111);

            uint32_t result = this->regs[m];
            if (d == 15)
            {

            }
            else
            {
                this->regs[d] = result;
                aspr.N = (result >> 31 ) & 0b1;
                aspr.Z = result == 0;
            }
            break;
        }

        case 0b11: // BX or BLX
        {
            bool sevenBit = (instruction >> 7) & 0b1;
            
            // BLX
            if (sevenBit)
            {
                printf("BLX\n");

                if (m == 15)
                {
                    std::cout << "Unpredictable behaviour!\n";
                    return;
                }

                uint32_t target = this->regs[m];
                uint32_t next_pc = this->regs[13] - 2;
                
                this->regs[14] = next_pc | 1; 

                this->espr.t = target & 0b1;
                this->regs[13] = target & ~0b1;
            }
            // BX
            else
            {
                if (m == 15)
                {
                    std::cout << "Unpredictable behaviour!\n";
                    return;
                }

                
                // this->regs[13] = this->regs[m];
            }
            // Thumb bit handling
            // pc = target & ~1u;
            // optionally check bit0 for state (Thumb only on M0)
            break;
        }
    }
}

uint32_t Cpu::fetch(void) const
{
    // little endians lsb first

    uint32_t pc = this->regs[15];


    // printf("pc: %d\n", pc);
    // uint32_t instr = this->memory[pc] |
    //     (this->memory[pc + 1] << 8) |
    //     (this->memory[pc + 2] << 16) |
    //     (this->memory[pc + 3] << 24);
    uint32_t instr = this->flash[pc] |
                    (this->flash[pc + 1] << 8) |
                    (this->flash[pc + 2] << 16) |
                    (this->flash[pc + 3] << 24);
    return instr;
}

uint8_t Cpu::currentCond(uint32_t instruction)
{
    return (instruction >> 7) & 0b1111;
}

uint32_t Cpu::shift(uint32_t value, SRType type, uint8_t amount, bit carry_in)
{
    Shift_c shifted = this->shift_c(value, type, amount, carry_in);
    return shifted.result;
}
        
uint32_t Cpu::sign_extend(uint32_t value, int bits)
{
    if (value & (1u << (bits - 1))) {
        value |= (~0u << bits);
    }
    return value;
}

DecodeImmShiftResult Cpu::decodeImmShift(uint8_t type, uint8_t imm5)
{
    SRType shift_t;
    uint8_t shift_n;
    switch(type)
    {
        case 0b00:
            shift_t = SRType_LSL;
            shift_n = imm5;
            break;
        case 0b01:
            shift_t = SRType_LSR;

            if (imm5 == 0x0)
            {
                shift_n = 32;
            }
            else
            {
                shift_n = imm5;
            }
            
            break;

        case 0b10:
            shift_t = SRType_ASR;

            if (imm5 == 0x0)
            {
                shift_n = 32;
            }
            else
            {
                shift_n = imm5;
            }
            
            break;

        case 0b11:

            if (imm5 == 0x0)
            {
                shift_t = SRType_RRX;
                shift_n = 1;
            }
            else
            {
                shift_t = SRType_ROR;
                shift_n = imm5;
            }
            break;
    }

    return {shift_t, shift_n};
}

Shift_c Cpu::shift_c(uint32_t value, SRType type, uint8_t amount, bit carry_in)
{
    if (amount == 0)
    {
        return {value, carry_in};
    }
    else
    {
        uint32_t result;
        bit carry_out;

        switch(type)
        {
            case SRType_ASR:
                break;
            case SRType_LSL:
                result = value << amount;
                carry_out = (value >> (32 - amount)) & 1;
                break;
            case SRType_LSR:
                result = value >> amount;
                carry_out = (value >> (amount - 1)) & 1;
                break;
            case SRType_ROR:
                break;
            case SRType_RRX:
                break;
            default:
                result = value;
                carry_out = carry_in;
                break;
        }

        return {result, carry_out};
    }
}
        
AddCarryResult Cpu::addWithCarry(uint32_t x, uint32_t y, bool carry_in)
{
    uint64_t unsigned_sum =
        (uint64_t)x +
        (uint64_t)y +
        (uint64_t)(carry_in ? 1 : 0);

    uint32_t result = (uint32_t)unsigned_sum;

    bool carry_out = (unsigned_sum >> 32) & 1;

    int64_t signed_sum =
        (int64_t)(int32_t)x +
        (int64_t)(int32_t)y +
        (carry_in ? 1 : 0);

    bool overflow =
        signed_sum > std::numeric_limits<int32_t>::max() ||
        signed_sum < std::numeric_limits<int32_t>::min();

    return { result, carry_out, overflow };
}

bool Cpu::inITBlock(void)
{
    return false;
}

bool Cpu::is32bitInstruction(uint8_t thumb_mode)
{
    if (thumb_mode == 0b11101 || thumb_mode == 0b11110 || thumb_mode == 0b11111)
    {
        return true;
    }

    return false;
}

void Cpu::HandleALUinstr(uint16_t instruction)
{
    fprintf(stderr, "handle ALU\n");
    uint8_t op = (instruction >> 6) & 0xF;
    uint8_t rm = (instruction >> 3) & 0x7;
    uint8_t rd = instruction & 0x7;

    uint32_t a = regs[rd];
    uint32_t b = regs[rm];
    uint32_t result;

    switch (op)
    {
        case 0x0: // AND
        {

            fprintf(stderr, "AND\n");
            uint8_t n, d = instruction & 0b111;
            uint8_t m = (instruction >> 3) & 0b111;
            uint32_t Rm = this->regs[m];

            uint8_t shift_n = 0;
            SRType type = SRType_LSL;    
            Shift_c shifted = this->shift_c(Rm, type, shift_n, this->aspr.C);                              

            uint32_t result = shifted.result & this->regs[n];

            this->regs[d] = result;
            this->aspr.N = shifted.result >> 31;
            this->aspr.C = shifted.carry_out;
            this->aspr.Z = shifted.result == 0x0;
            break;
        }

        case 0x1: // EOR
        {
            fprintf(stderr, "EOR\n");
            uint8_t n, d = instruction & 0b111;
            uint8_t m = (instruction >> 3) & 0b111;

            uint32_t Rm = this->regs[m];
            uint32_t Rn = this->regs[n];

            Shift_c shifted = this->shift_c(Rm, SRType_LSL, 0, aspr.C);

            uint32_t result = Rn ^ shifted.result;

            this->regs[d] = result;

            this->aspr.N = (result >> 31) & 0b1;
            this->aspr.C = shifted.carry_out;
            this->aspr.Z = result == 0x0;
            
            break;
        }

        case 0x2: // LSL (register)
        {
            fprintf(stderr, "LSL (register)\n");
            const uint8_t n = instruction & 0b111;
            const uint8_t d = instruction & 0b111;
            const uint8_t m = (instruction >> 3) & 0b111;
            const uint8_t shift_n = 0;

            const SRType type = SRType_LSL;

            const uint32_t Rm = this->regs[m];
            const uint32_t Rn = this->regs[n];
            const uint32_t shifted = this->shift(Rm, type, shift_n, this->aspr.C);

            const AddCarryResult carry_result = this->addWithCarry(Rn,shifted, this->aspr.C);

            this->regs[d] = carry_result.result;

            this->aspr.N = (carry_result.result >> 31) & 0b1;
            this->aspr.C = carry_result.carry_out;
            this->aspr.Z = carry_result.result == 0x0;
            this->aspr.V = carry_result.overflow;
            break;
        }

        case 0x3: // LSR (register)
        {
            fprintf(stderr, "LSR (register)\n");
            const uint8_t n = instruction & 0b111;
            const uint8_t d = instruction & 0b111;
            const uint8_t m = (instruction >> 3) & 0b111;
            const uint8_t shift_n = this->regs[m];

            const auto shifted = this->shift_c(this->regs[n], SRType_LSR, shift_n, this->aspr.C);

            this->regs[d] = shifted.result;
            
            this->aspr.N = (shifted.result >> 31) & 0b1;
            this->aspr.C = shifted.carry_out;
            this->aspr.Z = shifted.result == 0x0;
        
            break;
        }

        case 0x4: // ASR (register)
        {
            fprintf(stderr, "ASR (register)\n");
            const uint8_t n = instruction & 0b111;
            const uint8_t d = instruction & 0b111;
            const uint8_t m = (instruction >> 3) & 0b111;
            const uint8_t shift_n = this->regs[m];

            const auto shifted = this->shift_c(this->regs[n], SRType_ASR, shift_n, this->aspr.C);

            this->regs[d] = shifted.result;
            
            this->aspr.N = (shifted.result >> 31) & 0b1;
            this->aspr.C = shifted.carry_out;
            this->aspr.Z = shifted.result == 0x0;
            break;
        }

        case 0x5: // ADC
        {
            fprintf(stderr, "ADC\n");
            const uint8_t n = instruction & 0b111;
            const uint8_t d = instruction & 0b111;
            const uint8_t m = (instruction >> 3) & 0b111;

            const auto shifted = this->shift(this->regs[m], SRType_LSL, 0, this->aspr.C);
            const auto result = addWithCarry(this->regs[n], shifted, aspr.C);

            this->regs[d] = result.result;

            this->aspr.N = (result.result >> 31) & 0b1;
            this->aspr.C = result.carry_out;
            this->aspr.Z = result.result == 0x0;
            this->aspr.V = result.overflow;

            break;
        }
        case 0x6: // SBC
        {
            fprintf(stderr, "SBC\n");
            const uint8_t n = instruction & 0b111;
            const uint8_t d = instruction & 0b111;
            const uint8_t m = (instruction >> 3) & 0b111;
            
            const auto shifted = this->shift(this->regs[m], SRType_LSL, 0, aspr.C);

            const auto result = addWithCarry(this->regs[n], ~shifted , aspr.C);

            this->regs[d] = result.result;

            this->aspr.N = (result.result >> 31) & 0b1;
            this->aspr.Z = result.result == 0x0;
            this->aspr.C = result.carry_out;
            this->aspr.V = result.overflow;

            break;
        }

        case 0x7: // ROR
        {
            fprintf(stderr, "ROR\n");
            uint8_t d, n = instruction & 0b111;
            uint8_t m = (instruction >> 3) & 0b111;

            uint8_t shift_n = this->regs[m] & 0xFF;

            auto result = shift_c(this->regs[n], SRType_ROR, shift_n, aspr.C);

            this->regs[d] = result.result;

            this->aspr.N = (result.result >> 31) & 0b1;
            this->aspr.Z = result.result == 0x0;
            this->aspr.C = result.carry_out;
            break;
        }

        case 0x8: // TST (no write)
        {
            fprintf(stderr, "TST\n");
            uint8_t n, d = instruction & 0b111;
            uint8_t m = (instruction >> 3) & 0b111;

            const auto shifted = this->shift_c(this->regs[m], SRType_LSL, 0, aspr.C);     

            const uint32_t result = this->regs[n] & shifted.result;

            this->aspr.N = result >> 31;
            this->aspr.C = shifted.carry_out;
            this->aspr.Z = result == 0x0;
            break;
        }

        case 0x9: // NEG
        {
            fprintf(stderr, "NEG\n");
            uint8_t d = instruction & 0b111;
            uint8_t n = (instruction >> 3) & 0b111;

            auto result = addWithCarry(~(this->regs[n]), 0, true);

            this->regs[d] = result.result;

            this->aspr.N = (result.result >> 31) & 0b1;
            this->aspr.Z = result.result == 0x0;
            this->aspr.C = result.carry_out;
            this->aspr.V = result.overflow;
            break;
        }

        case 0xA: // CMP (no write)
        {
            fprintf(stderr, "CMP (Register)\n");

            uint8_t n = instruction & 0b111;
            uint8_t d = n;
            uint8_t m = (instruction >> 3) & 0b111;
            
            fprintf(stderr, "R%d: %x\n", n, this->regs[n]);
            fprintf(stderr, "R%d: %x\n", m, this->regs[m]);

            const uint32_t shifted = this->shift(this->regs[m], SRType_LSL, 0, this->aspr.C); 
            const auto result = addWithCarry(this->regs[n], ~shifted, 1);
            
            this->aspr.N = (result.result >> 31) & 0b1;
            this->aspr.V = result.overflow;
            this->aspr.C = result.carry_out;
            this->aspr.Z = result.result == 0x0;
            break;
        }

        case 0xB: // CMN (no write)
        {
            fprintf(stderr, "CMN (Register)\n");
            uint8_t n, d = instruction & 0b111;
            uint8_t m = (instruction >> 3) & 0b111;
            
            const uint32_t shifted = this->shift(this->regs[m], SRType_LSL, 0, this->aspr.C); 

            const auto result = addWithCarry(this->regs[n], shifted, 0);
            
            this->aspr.N = (result.result >> 31) & 0b1;
            this->aspr.V = result.overflow;
            this->aspr.C = result.carry_out;
            this->aspr.Z = result.result == 0x0;

            break;
        }

        case 0xC: // ORR
        {
            fprintf(stderr, "ORR (Register)\n");
            uint8_t n, d = instruction & 0b111;
            const uint8_t m = (instruction >> 3) & 0b111;
    
            const Shift_c shifted = this->shift_c(this->regs[m], SRType_LSL, 0, aspr.C);

            const uint32_t result = this->regs[n] | shifted.result;

            this->regs[d] = result;
            this->aspr.N = (result >> 31) & 0b1;
            this->aspr.C = shifted.carry_out;
            this->aspr.Z = result == 0x0;
            break;
        }
        case 0xD: // MUL
        {
            fprintf(stderr, "MUL\n");

            uint8_t n, d = instruction & 0b111;
            uint8_t m = (instruction >> 3) & 0b111;

            uint32_t operand1 = this->regs[n];
            uint32_t operand2 = this->regs[m];

            uint64_t result = operand2 * operand1;

            this->regs[d] = (uint32_t) result;

            this->aspr.N = result >> 31;

            this->aspr.Z = (uint32_t) result == 0;
            break;
        }

        case 0xE: // BIC
        {
            fprintf(stderr, "BIC\n");
            uint8_t n, d = instruction & 0b111;
            uint8_t m = (instruction >> 3) & 0b111;
    
            Shift_c shifted = this->shift_c(this->regs[m], SRType_LSL, 0, this->aspr.C);                              

            uint32_t result = shifted.result & ~(this->regs[n]);

            this->regs[d] = result;

            this->aspr.N = (shifted.result >> 31) & 0b1;
            this->aspr.C = shifted.carry_out;
            this->aspr.Z = shifted.result == 0x0;
        }

        case 0xF: // MVN
        {
            fprintf(stderr, "MVN");
            uint8_t n, d = instruction & 0b111;
            uint8_t m = (instruction >> 3) & 0b111;
    
            Shift_c shifted = this->shift_c(this->regs[m], SRType_LSL, 0, this->aspr.C);                              

            uint32_t result = ~(shifted.result);

            this->regs[d] = result;

            this->aspr.N = (shifted.result >> 31) & 0b1;
            this->aspr.C = shifted.carry_out;
            this->aspr.Z = shifted.result == 0x0;
            break;
        }
    }
}

void Cpu::handleLoadStoreHalf(uint16_t instr) 
{
    bool L    = (instr >> 11) & 0x1;
    uint8_t imm5 = (instr >> 6) & 0x1F;
    uint8_t n   = (instr >> 3) & 0x7;
    uint8_t t   = instr & 0x7;

    if (L) 
    {
        uint32_t imm32 = (uint32_t)(imm5 << 2);

        bool index = true;
        bool add = true;
        bool wback = false;

        uint32_t Rn = this->regs[n];

        uint32_t offset_addr = add ? Rn + imm32 : Rn - imm32;

        uint32_t address = index  ? offset_addr : Rn;

        this->regs[t] = read16(address);
    } 
    else 
    {
        uint32_t imm32 = (uint32_t)(imm5 << 2);

        bool index = true;
        bool add = true;
        bool wback = false;

        uint32_t Rn = this->regs[n];

        uint32_t offset_addr = add ? Rn + imm32 : Rn - imm32;

        uint32_t address = index  ? offset_addr : Rn;

        fprintf(stderr, "addr: %x\n", address);
        write16(address, (uint16_t) this->regs[t]);
    }
}

void Cpu::handleLoadStoreImm(uint16_t instr) 
{
    uint8_t op = (instr >> 11) & 0x3;  // B/L combo
    uint8_t imm5 = (instr >> 6) & 0x1F;
    uint8_t n = (instr >> 3) & 0x7;
    uint8_t t = instr & 0x7;

    uint32_t addr;

    switch (op) 
    {
        case 0b00: // STR (word)
        {
            
            fprintf(stderr, "STR (IMMEDIATE)\n");
            uint32_t imm32 = (uint32_t)(imm5 << 2);
            uint32_t offset_addr = this->regs[n] + imm32;
            uint32_t address = offset_addr;
            

            fprintf(stderr,"Rn = r%d = 0x%08x\n", n, regs[n]);
            fprintf(stderr,"Rt = r%d = 0x%08x\n", t, regs[t]);
            fprintf(stderr,"Address = 0x%08x\n", address);

            write32(address, this->regs[t]);

            break;
        }

        case 0b01: // LDR (word)
        {

            fprintf(stderr,"LDR (IMMEDIATE)\n");

            uint32_t imm32 = imm5 << 2;

            uint32_t address = regs[n] + imm32;

            fprintf(stderr,"Rn = r%d = 0x%08x\n", n, regs[n]);
            fprintf(stderr,"Rt = r%d = 0x%08x\n", t, regs[t]);
            fprintf(stderr,"Address = 0x%08x\n", address);

            uint32_t value = read32v2(address);

            fprintf(stderr,"Loaded value = 0x%08x\n", value);

            regs[t] = value;

            break;
        }

        case 0b10: // STRB
        {
            fprintf(stderr,"STRB (IMMEDIATE)\n");
            uint32_t imm32 = (uint32_t)(imm5 << 2);

            bool index = true;
            bool add = true;
            bool wback = false;

            uint32_t Rn = this->regs[n];

            uint32_t offset_addr = add ? Rn + imm32 : Rn - imm32;

            uint32_t address = index  ? offset_addr : Rn;

            write8(address, this->regs[t]);

            break;
        }

        case 0b11: // LDRB
        {
           fprintf(stderr,"LDRB (IMMEDIATE)\n");
            uint32_t imm32 = (uint32_t)(imm5 << 2);

            bool index = true;
            bool add = true;
            bool wback = false;

            uint32_t Rn = this->regs[n];

            uint32_t offset_addr = add ? Rn + imm32 : Rn - imm32;

            uint32_t address = index  ? offset_addr : Rn;

            this->regs[t] = (uint32_t) read8(address);

            break;
        }
    }
}

void Cpu::handleLDRLiteral(uint16_t instruction)
{
   fprintf(stderr,"LDR Literal\n");
    uint8_t t = (instruction >> 8) & 0b111;
    uint8_t imm8 = (instruction) & 0xFF;
    uint32_t imm32 = (uint32_t)(imm8 << 2); 
    uint32_t base = (this->regs[15] + 4) & ~0x3;
    uint32_t address = base + imm32;

    fprintf(stderr,"imm: %d\n", imm32);
    fprintf(stderr,"dest reg: %x\n", t);
    fprintf(stderr,"Address %x\n", imm32 + base);

    this->regs[t] = read32v2(address);
}

void Cpu::handleShiftImmediate(uint16_t instr)
{
    fprintf(stderr,"LSL (Immediate)\n");
    uint8_t m = (instr >> 3) & 0x7;
    uint8_t d = (instr) & 0x7;
    uint8_t  imm5 = (instr >> 6) & 0b11111;


    fprintf(stderr, "imm5: %d\n", imm5);
    if (imm5 == 0x0)
    {
        fprintf(stderr, "SEE MOV\n");
        // See MOV
    }
    auto result = decodeImmShift(0b00, imm5);

    auto shifted = shift_c(this->regs[m], SRType_LSL, result.n, aspr.C);
    this->regs[d] = shifted.result;

    this->aspr.N = (shifted.result >> 31) & 0b1;
    this->aspr.C = shifted.carry_out;
    this->aspr.Z = shifted.result == 0x0;

    this->regs[15] += 2;
}

std::pair<uint32_t, bool> Cpu::LSL_C(uint32_t x, int shift)
{
    // shift > 0
    uint32_t result = x << shift;
    bool carry = (x >> (32 - shift)) & 1;
    return {result, carry};
}

uint32_t Cpu::LSL(uint32_t x, int shift) 
{
    if (shift == 0) return x;
    return x << shift;
}

std::pair<uint32_t, bool> Cpu::ROR_C(uint32_t x, int shift) 
{
    int m = shift % 32;

    uint32_t result = (x >> m) | (x << (32 - m));
    bool carry = (result >> 31) & 1;

    return {result, carry};
}

uint32_t Cpu::ROR(uint32_t x, int shift) {
    if (shift == 0) return x;
    return (x >> (shift % 32)) | (x << (32 - (shift % 32)));
}

std::pair<uint32_t, bool> Cpu::LSR_C(uint32_t x, int shift) 
{
    // shift > 0
    uint32_t result = x >> shift;
    bool carry = (x >> (shift - 1)) & 1;
    return {result, carry};
}

std::pair<uint32_t, bool> Cpu::ASR_C(uint32_t x, int shift) {
    // shift > 0
    int32_t sx = (int32_t)x;

    uint32_t result = (uint32_t)(sx >> shift);
    bool carry = (x >> (shift - 1)) & 1;

    return {result, carry};
}

std::pair<uint32_t, bool> Cpu::RRX_C(uint32_t x, bool carry_in) {
    uint32_t result = (carry_in << 31) | (x >> 1);
    bool carry_out = x & 1;

    return {result, carry_out};
}

uint32_t Cpu::RRX(uint32_t x, bool carry_in) {
    return (carry_in << 31) | (x >> 1);
}

uint32_t Cpu::ASR(uint32_t x, int shift) {
    if (shift == 0) return x;
    return (uint32_t)((int32_t)x >> shift);
}

uint32_t Cpu::LSR(uint32_t x, int shift) 
{
    if (shift == 0) return x;
    return x >> shift;
}

void Cpu::BXWritePC(uint32_t address) {

    // to be implemented
}

uint32_t Cpu::read32v2(uint32_t address) const
{
    if (address < RAM_BASE)
    {        address = address - FLASH_BASE;
        uint32_t data = this->flash[address] |
            (this->flash[address + 1] << 8) |
            (this->flash[address + 2] << 16) |
            (this->flash[address + 3] << 24);

        fprintf(stderr,"Reading: Flash\n");
        fprintf(stderr,"Reading: Flash Address: %d\n", address);
        fprintf(stderr,"FLASH DATA: %x\n", data);   
        
        return data;
    }
    else
    {
        fprintf(stderr, "Reading: RAM\n");

        address = address - RAM_BASE;

        fprintf(stderr, "address - RAM_BASE: %x\n", address);
        uint32_t data = this->ram[address] |
            (this->ram[address + 1] << 8) |
            (this->ram[address + 2] << 16) |
            (this->ram[address + 3] << 24);

        return data;
    }
}

void Cpu::handleMovCmpAddSub(uint16_t instr)
{
    uint8_t op  = (instr >> 11) & 0b11;
    uint8_t d  = (instr >> 8)  & 0b111;
    uint8_t imm8 = instr & 0xFF;

    switch (op)
    {
        case 0b00: // MOVS Rd, #imm
        {
            fprintf(stderr, "MOV (imm)\n");
            uint32_t imm32 = (uint32_t) imm8;
            this->regs[d] = imm32;

            aspr.N = (imm32 >> 31) & 0b1;
            // aspr.C = 
            aspr.Z = imm32 == 0;

            fprintf(stderr, "R%d: %x\n", d, this->regs[d]);
            break;
        }

        case 0b01: // CMP Rd, #imm
        {

            fprintf(stderr, "CMP (imm)\n");
            uint32_t imm32 = (uint32_t) imm8;
            // d == n here
            const auto result = addWithCarry(this->regs[d], ~imm32, 1);

            aspr.N = (result.result >> 31) & 0b1;
            aspr.C = result.carry_out;
            aspr.Z = imm32 == 0;
            aspr.V = result.overflow;
            break;
        }

        case 0b10: // ADDS Rd, #imm
        {

            break;
        }

        case 0b11: // SUBS Rd, #imm
        {


            break;
        }
    }
}

void Cpu::handleMisc(uint16_t instr) 
{

    // ---- ADD / SUB SP ----
    if ((instr & 0b1111111100000000) == 0b1011000000000000) 
    {
        bool S = (instr >> 7) & 1;     // 0=ADD, 1=SUB
        const uint8_t imm7 = instr & 0b1111111;
        const uint32_t imm32 = ((uint32_t) imm7 << 2);

        if (S)
        {
            fprintf(stderr, "SUB\n");
            const auto result = this->addWithCarry(getSP(), ~imm32, 1);
            this->regs[13] = result.result;
        }
        else
        {
            fprintf(stderr, "ADD\n");
            const auto result = this->addWithCarry(getSP(), imm32, 0);
            this->regs[13] = result.result;
        }

        return;
    }

    // // ---- PUSH / POP ----
    if ((instr & 0b1111000000000000) == 0b1011000000000000) {
        bool L = (instr >> 11) & 1;   // 0=PUSH, 1=POP
        bool R = (instr >> 8) & 1;    // LR (push) / PC (pop)
        uint8_t register_list = instr & 0xFF;

        if (L) 
        {
            fprintf(stderr, "POP\n");
            uint32_t registers = register_list << 7;

            if (std::bitset<32>(registers).count() < 1)
            {
                return;
            }   

            uint32_t address = this->getSP();

            for (uint8_t i = 0; i < 8 ; i++)
            {
                // Isolate the ith bit
                if (registers & (0b1 << i))
                {
                    this->regs[i] = this->read32(address);
                    address += 4;
                }
            }

            if (registers & (0b1 << 15))
            {
                this->BXWritePC(address);
            }

            this->regs[13] += (4 * std::bitset<32>(registers).count());
        } 
        else 
        {
            fprintf(stderr, "PUSH\n");
            bool M = (instr >> 8) & 1; 

            //             0 : M : 000 000 : 0000 0000
            //                               
            uint32_t registers =  register_list | (M << 13);


            uint32_t address = getSP() - (4 * std::bitset<32>(registers).count());

            fprintf(stderr, "handleMisc(): adddress: %x\n", address);

            for (int i = 0; i < 15; i++)
            {
                if (register_list & (1 << i))
                {
                    write32(address, regs[i]);
                    address += 4;
                }
            }

            this->regs[13] -= (4 * std::bitset<32>(registers).count());
        }
        return;
    }

    // ---- EXTEND (UXTB/SXTB/UXTH/SXTH) ----
    if ((instr & 0b1111110000000000) == 0b1011001000000000) {
        uint8_t op = (instr >> 6) & 0x3;
        uint8_t m = (instr >> 3) & 0x7;
        uint8_t d = instr & 0x7;

        switch (op) 
        {
            case 0:
            {
                uint8_t rotation = 0;
                const auto result = this->ROR(this->regs[m], rotation);
                
                this->regs[d] = sign_extend((uint16_t) result, 32);

                break; 
                // SXTH
            }
            case 1:
            {
                uint8_t rotation = 0;
                const auto result = this->ROR(this->regs[m], rotation);
                
                this->regs[d] = sign_extend((uint8_t) result, 32);
                break; // SXTB;
            }
            
            case 2:               
            {
                uint8_t rotation = 0;
                const auto result = this->ROR(this->regs[m], rotation);
                
                this->regs[d] = (uint16_t) result;
                break; // UXTH;
            }
            case 3:                     
            {
                uint8_t rotation = 0;
                const auto result = this->ROR(this->regs[m], rotation);
                
                this->regs[d] = (uint8_t )result;
                break; // UXTB;
            }
        }
        return;
    }

    // REV family
    if ((instr & 0b1111110000000000) == 0b1011101000000000) {
        uint8_t op = (instr >> 6) & 0x3;
        uint8_t m = (instr >> 3) & 0x7;
        uint8_t d = instr & 0x7;

        // uint32_t v = regs[rm];

        switch (op) 
        {
            case 0: // REV
            {   
                uint32_t Rm = this->regs[m];
                
                uint32_t result = 
                (Rm & 0xFF) << 24 | 
                ((Rm >> 8) & 0xFF) << 16 | 
                ((Rm >> 16) & 0xFF) << 8 |
                ((Rm >> 24) & 0xFF);

                break;
            }

            case 1: // REV16
            {
                uint32_t Rm = regs[m];

                uint32_t result =
                    ((Rm & 0x00FF00FF) << 8) |
                    ((Rm & 0xFF00FF00) >> 8);

                regs[d] = result;
                break;
            }

            case 3: // REVSH
            {
                uint32_t Rm = regs[m];

                // Extract low 16 bits
                uint16_t half = Rm & 0xFFFF;

                // Swap bytes
                uint16_t swapped = (half >> 8) | (half << 8);

                // Sign extend to 32-bit
                int32_t result = (int16_t)swapped;

                regs[d] = (uint32_t)result;
                break;
            }
        }
        return;
    }
}
    

void Cpu::handleAddr(uint16_t instr) 
{
    bool S    = (instr >> 11) & 0x1;
    uint8_t d   = (instr >> 8) & 0x7;
    uint8_t imm8 = instr & 0xFF;

    if (S) 
    {
        // ADD (SP plust immediate)
        uint32_t imm32 = ((uint32_t) imm8) << 2;
        uint32_t SP = this->regs[13];

        auto result = this->addWithCarry(SP, imm32, 0);
        this->regs[d] = result.result;
    } 
    else 
    {
        bool add = true;
        // ADR
        uint32_t imm32 = ((uint32_t) imm8) << 2;
        uint32_t result = (this->regs[15] + 4) & ~0x3;
        this->regs[d] = result;
    }
}

void Cpu::handleLoadStoreReg(uint16_t instr)
{
    fprintf(stderr, "HANDLE STORE REG\n");
    uint8_t op = (instr >> 9) & 0x7;   // L/B/H combo
    uint8_t m = (instr >> 6) & 0x7;
    uint8_t n = (instr >> 3) & 0x7;
    uint8_t t = instr & 0x7;

    switch (op) 
    {
        case 0b000: // STR
        {
            bool index = true;
            bool add = true;
            bool wback = false;                  
            const auto offset = shift(this->regs[m], SRType_LSL, 0, aspr.C);
            uint32_t address = this->regs[n] + offset;
            write32(address, this->regs[t]);
            
            break;
        }
        case 0b001: // STRH
        {
            bool index = true;
            bool add = true;
            bool wback = false;                  
            const auto offset = shift(this->regs[m], SRType_LSL, 0, aspr.C);
            uint32_t address = this->regs[n] + offset;
            
            write16(address, this->regs[t]);
            
            break;
        }
        case 0b010: // STRB
        {   
            bool index = true;
            bool add = true;
            bool wback = false;                  
            const auto offset = shift(this->regs[m], SRType_LSL, 0, aspr.C);
            uint32_t address = this->regs[n] + offset;
            
            write8(address, this->regs[t]);
            break;
        }
        case 0b011: // LDRSB
        {
            bool index = true;
            bool add = true;
            bool wback = false;

            uint32_t Rn = this->regs[n];

            const auto offset = shift(this->regs[m], SRType_LSL, 0, aspr.C);

            uint32_t offset_addr =  add ? Rn + offset : Rn - offset;
            uint32_t address = index ? offset_addr : Rn;

            uint8_t data = this->read8(address);

            this->regs[t] = sign_extend(data, 32); 
            break;   
        }
        case 0b100: // LDR
        {

            fprintf(stderr, "LDR (Register)\n");
            bool index = true;
            bool add = true;
            bool wback = false;

            uint32_t Rn = this->regs[n];

            const auto offset = shift(this->regs[m], SRType_LSL, 0, aspr.C);

            uint32_t offset_addr =  add ? Rn + offset : Rn - offset;
            uint32_t address = index ? offset_addr : Rn;

            this->regs[t] = this->read32(address);
            break;
        }
        case 0b101: // LDRH
        {
            bool index = true;
            bool add = true;
            bool wback = false;

            uint32_t Rn = this->regs[n];

            const auto offset = shift(this->regs[m], SRType_LSL, 0, aspr.C);

            uint32_t offset_addr =  add ? Rn + offset : Rn - offset;
            uint32_t address = index ? offset_addr : Rn;

            this->regs[t] = (uint32_t) this->read16(address);
    
            break;
        }
        case 0b110: // LDRB
        {
            bool index = true;
            bool add = true;
            bool wback = false;

            uint32_t Rn = this->regs[n];

            const auto offset = shift(this->regs[m], SRType_LSL, 0, aspr.C);

            uint32_t offset_addr =  add ? Rn + offset : Rn - offset;
            uint32_t address = index ? offset_addr : Rn;

            this->regs[t] = (uint32_t) this->read8(address);
    
            break;
        }
        case 0b111: // LDRSH
        {
            bool index = true;
            bool add = true;
            bool wback = false;

            uint32_t Rn = this->regs[n];

            const auto offset = shift(this->regs[m], SRType_LSL, 0, aspr.C);

            uint32_t offset_addr =  add ? Rn + offset : Rn - offset;
            uint32_t address = index ? offset_addr : Rn;

            this->regs[t] = sign_extend(this->read16(address), 32);
    
            break;
        }
    }
}

void Cpu::handleSpRelative(uint16_t instr) {
    bool L    = (instr >> 11) & 0x1;
    uint8_t t   = (instr >> 8) & 0x7;
    uint8_t imm8 = instr & 0xFF;

    if (L) 
    {
        uint32_t imm32 = (uint32_t)(imm8 << 2); 
        uint8_t n = 13;
        bool index = true;
        bool add = true;
        bool wback = false;

        uint32_t Rn = this->regs[n];

        uint32_t offset_addr =  add ? Rn + imm32 : Rn - imm32;
        uint32_t address = index ? offset_addr : Rn;

        this->regs[t] = read32(address);
    } 
    else 
    {
        uint8_t n = 13;

        uint32_t imm32 = (uint32_t)(imm8 << 2);

        bool index = true;
        bool add = true;
        bool wback = false;

        uint32_t Rn = this->regs[n];

        uint32_t offset_addr = add ? Rn + imm32 : Rn - imm32;

        uint32_t address = index  ? offset_addr : Rn;

        write32(address, this->regs[t]);
    }
}

void Cpu::handleMultiple(uint16_t instr)
{
    bool L = (instr >> 11) & 1;
    uint8_t n = (instr >> 8) & 0x7;
    uint8_t register_list = instr & 0xFF;

    if (L) 
    {
        // ---- LDMIA (load multiple) ----
        uint16_t registers = (uint16_t) register_list;

        bool wback = (registers & 0b1 << n) == 0;

        if (std::bitset<32>(registers).count() < 1)
        {
            std::cerr << "unpredictable" << std::endl;
            return;
        }

        uint32_t address = this->regs[n];

        for (uint8_t i = 0; i < 8 ; i++)
        {
            // Isolate the ith bit

            if (registers & (0b1 << i))
            {
                this->regs[i] = this->read32(address);
                address += 4;
            }
        }

        if (wback && (registers & 0b1 << n) == 0)
        {
            this->regs[n] = this->regs[n] + (4 * std::bitset<32>(registers).count());
        }
    } 
    else 
    {
        if (std::bitset<16>(register_list).count() < 1)
        {
            std::cerr << "unpredictable" << std::endl;
            return;
        }

        bool wback = true;


        uint8_t position = 0;

        if (register_list == 0)
        {
            position = 16;
        }
        else
        {
            uint16_t temp = register_list;
            while ((temp & 0b1) == 1)
            {
                temp >>= 1;
                position++;
            }
        }

        uint32_t address = this->regs[n];

        for (uint8_t i = 0; i < 15 ; i++)
        {
            // Isolate the ith bit

            if (register_list & (0b1 << i))
            {
                if (i == n && wback && i != position)
                {
                    write32(address, 0xDEADBEEF);
                }
                else
                {
                    write32(address, this->regs[i]);
                }

                address += 4;
            }
        }

        if (wback)
        {
            registers[n] = this->regs[n] + (4 * std::bitset<32>(register_list).count());
            this->regs[n] = this->regs[n] + (4 * std::bitset<32>(register_list).count());
        }
    }
}

void Cpu::handleCondBranch(uint16_t instr)
{
    fprintf(stderr, "Handle cond\n");
    uint8_t cond = (instr >> 8 ) & 0xF;
    uint8_t imm8 = (instr & 0xFF);

    if (cond == 0b1110)
    {

    }
    else if (cond == 0b1111)
    {

    }

    int32_t imm32 = sign_extend(imm8 << 1, 9);

    if (conditionPassed(cond))
    {
        regs[15] = (regs[15] + 4 + imm32) & ~1;
    }
    else
    {
        this->regs[15] += 2;
    }
}

void Cpu::handleUncondBranch(uint16_t instr)
{
    fprintf(stderr, "B (Branch)\n");
    uint32_t imm11 = instr & 0x7FF;

    int32_t imm32 = sign_extend(imm11 << 1, 12);

    uint32_t next_pc = this->regs[15] + 4;

    this->regs[15] = next_pc + imm32;
}

void Cpu::decode(void)
{
    uint32_t instruction = this->fetch();
    uint8_t format_id = (instruction >> 11) & 0x1F;
    
    std::cerr << "Instruction:" << std::hex << (uint16_t) instruction << std::endl;
    
    if (is32bitInstruction(format_id))
    {
         std::cerr << "Intrusction:" << std::bitset<32>(instruction) << std::endl;
        switch (format_id)
        {
            case 0b11101:

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
                    uint8_t d = second & 0x7;
                    uint8_t sysm = first & 0xFF;

                    switch ((sysm >> 3) & 0x1F)
                    {
                        case 0:
                        {
                            // APSR
                            if ((sysm & 0x7) == 0)
                            {
                                regs[d] = xpsr;
                            }
                            break;
                        }

                        case 1:
                        {
                            switch (sysm & 0x7)
                            {
                                case 0:
                                    regs[d] = msp;
                                    break;

                                case 1:
                                    regs[d] = psp;
                                    break;
                            }
                            break;
                        }

                        case 2:
                        {
                            switch (sysm & 0x7)
                            {
                                case 0:
                                    // regs[d] = primask;
                                    break;

                                case 4:
                                    // regs[d] = control;
                                    break;
                            }
                            break;
                        }
                    }
                }
                else if ((op1 & 0b1111111) == 0b0111011 && ((op2 & 0b001) | (op2 & 0b100)) == 0b000)
                {

                }
                else if ((op1 & 0b1111110) == 0b0111110 &&  ((op2 & 0b001) | (op2 & 0b100)) == 0b000)
                {

                }
                else if ((op1 & 0b1111111) == 0b1111111 && op2 == 0b010)
                {
                    // undefined
                }
                else if ( ((op2 & 0b001) | (op2 & 0b100)) == 0b101)
                {

                    fprintf(stderr,"BRANCH AND LINK\n");
                    // Branch and Link

                    bool S  = (first >> 10) & 1;

                    uint16_t imm10 = first & 0x03FF;

                    bool J1 = (second >> 13) & 1;
                    bool J2 = (second >> 11) & 1;

                    uint16_t imm11 = second & 0x07FF;

                    bool I1 = !(J1 ^ S);
                    bool I2 = !(J2 ^ S);

                    uint32_t imm25 =
                        (S  << 24) |
                        (I1 << 23) |
                        (I2 << 22) |
                        (imm10 << 12) |
                        (imm11 << 1);

                    int32_t  imm32 = sign_extend(imm25, 25);

                    uint32_t next = regs[15] + 4;

                    regs[14] = next | 1;

                    regs[15] = next + imm32;
                }
                break;

            }
            case 0b11111:
                // 32 bit thumb mode
                // 
                break;

        }
    }
    else
    {
        std::cerr << "Intrusction:" << std::bitset<16>((uint16_t)instruction) << std::endl;
        std::cerr << "Intrusction:" << std::hex << ((uint16_t)instruction) << std::endl;
    
        InstrClass instructionClass = classify(instruction);
    
        std::cerr << "Classified" << std::endl;

        std::cerr << static_cast<int>(instructionClass) << std::endl;
        switch (instructionClass)
        {
            case InstrClass::SHIFT_IMM:
                handleShiftImmediate(instruction);
                std::cerr << "SHIFT_IMM\n";
                break;

            case InstrClass::ADD_SUB:
                handleAddSub(instruction);
                std::cerr << "ADD_SUB\n";
                break;

            case InstrClass::MOV_CMP_ADD_SUB:
                handleMovCmpAddSub(instruction);
                std::cerr << "MOV_CMP_ADD_SUB\n";
                this->regs[15] += 2;
                break;

            case InstrClass::ALU:
                HandleALUinstr(instruction);
                this->regs[15] += 2;
                std::cerr << "ALU\n";
                break;

            case InstrClass::HI_REG:
                handleSpecialInstructions(instruction);
                this->regs[15] += 2;
                std::cerr << "HI_REG\n";
                break;

            case InstrClass::LDR_LITERAL:
                std::cerr << "LDR_LITERAL\n";
                handleLDRLiteral(instruction);
                this->regs[15] += 2;
                break;

            case InstrClass::LOAD_STORE_REG:
                handleLoadStoreReg(instruction);
                std::cerr << "LOAD_STORE_REG\n";
                this->regs[15] += 2;
                break;

            case InstrClass::LOAD_STORE_IMM:
                handleLoadStoreImm(instruction);
                this->regs[15] += 2;
                std::cerr << "LOAD_STORE_IMM\n";
                break;

            case InstrClass::LOAD_STORE_HALF:
                handleLoadStoreHalf(instruction);
                this->regs[15] += 2;
                std::cerr << "LOAD_STORE_HALF\n";
                break;

            case InstrClass::SP_REL:
                handleSpRelative(instruction);
                this->regs[15] += 2;
                break;

            case InstrClass::ADDR:
                std::cerr << "ADDR\n";
                handleAddr(instruction);
                this->regs[15] += 2;
                break;

            case InstrClass::MISC:
                std::cerr << "MISC\n";
                handleMisc(instruction);
                this->regs[15] += 2;
                break;

            case InstrClass::MULTIPLE:
                handleMultiple(instruction);
                this->regs[15] += 2;
                std::cerr << "MULTIPLE\n";
                break;

            case InstrClass::COND_BRANCH:
                handleCondBranch(instruction);
                std::cerr << "COND_BRANCH\n";
                break;

            case InstrClass::SVC:
                std::cerr << "SVC\n";
                break;

            case InstrClass::UNCOND_BRANCH:
                handleUncondBranch(instruction);
                std::cerr << "UNCOND_BRANCH\n";
                break;

            case InstrClass::UNKNOWN:
            default:
                std::cerr << "UNKNOWN\n";
                break;
        }
        // this->print_state();        
    }
}


void Cpu::print_state(void) const
{
    printf("\n--- CPU STATE ---\n");

    for(uint8_t i = 0; i < 16; i++)
    {
        printf("r%-2d: 0x%08X    ", i, this->regs[i]);
        
        // Every 4 registers, print a newline to maintain the grid
        if ((i + 1) % 4 == 0) {
            printf("\n");
        }
    }

    printf("-----------------\n");
    printf("FLAGS: [ N:%d | Z:%d | C:%d | V:%d ]\n", 
            this->aspr.N, this->aspr.Z, this->aspr.C, this->aspr.V);
    printf("-----------------\n\n");
}
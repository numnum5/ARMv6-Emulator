#include "cpu.hpp"

constexpr uint32_t UART_BASE = 0x40000000;
constexpr uint32_t UART_DR   = UART_BASE + 0x0;
constexpr uint32_t UART_SR   = UART_BASE + 0x4;

Cpu::Cpu(size_t ram_size, size_t flash_size) : ram(ram_size), flash(flash_size)
{
    this->reset();
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
        result = xpsr.N();
        break;

    case 0b001:
        result = xpsr.C();
        break;

    case 0b010:
        result = xpsr.N();
        break;

    case 0b011:
        result = xpsr.V();
        break;

    case 0b100:
        result = xpsr.C() && !xpsr.Z();
        break;

    case 0b101:
        result = (xpsr.N() == xpsr.V());
        break;

    case 0b110:
        result = (xpsr.N() == xpsr.V()) && !xpsr.Z();
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

void Cpu::write32(uint32_t address, uint32_t value)
{
    // System register access
    if (address >= 0xE0000000 && address <= 0xE00FFFFF)
    {
        this->scs.write32(address, value);
        fprintf(stderr, "write32: Address: %x\n", address);
        return;
    }

    // UART semihosting access
    if(address == UART_DR)
    {
        if ((value & 0xFF) == '\0')
        {
            while(1);
        }

        printf("%c", value & 0xFF);
        fprintf(stderr, "Char value: %c\n", value & 0xFF);
        fflush(stdout);

        // while(1);//
        return;
    }

    // Normal access
    address = address - RAM_BASE;

    if (address == 0x00000fec)
    {
        printf("THIS IS WRITING TO 0x00000fec\n");
        // while(1);
    }

    fprintf(stderr, "write32: Address: %x\n", address);
    this->ram[address] = value & 0xFF;
    this->ram[address + 1] = (value >> 8) & 0xFF;
    this->ram[address + 2] = (value >> 16) & 0xFF;
    this->ram[address + 3] = (value >> 24) & 0xFF;
}

void Cpu::writeFlash16(uint32_t address, uint16_t value)
{
    printf("write16(): address: %x\n", address);
    address = address - FLASH_BASE;
    this->flash[address] = value & 0xFF;
    this->flash[address + 1] = (value >> 8) & 0xFF;
}

void Cpu::writeFlash32(uint32_t address, uint16_t value)
{
    address = address - FLASH_BASE;
    this->flash[address] = value & 0xFF;
    this->flash[address + 1] = (value >> 8) & 0xFF;
    this->flash[address + 2] = (value >> 16) & 0xFF;
    this->flash[address + 3] = (value >> 24) & 0xFF;
}

void Cpu::write16(uint32_t address, uint16_t value)
{
 

     if (address == 0x00000fec)
    {
        printf("THIS IS WRITING TO 0x00000fec\n");



        // while(1);
    }
    printf("write16(): address: %x\n", address);
    address = address - RAM_BASE;
    this->ram[address] = value & 0xFF;
    this->ram[address + 1] = (value >> 8) & 0xFF;
}

void Cpu::write8(uint32_t address, uint8_t value)
{
    fprintf(stderr, "write8(): address: %x, value: %c\n", address, value);

    address = address - RAM_BASE;
    this->ram[address] = value;
}

uint8_t Cpu::read8(uint32_t address) const
{
    std::cerr << "read8(): " << std::hex << address << std::endl;
    if (address < RAM_BASE)
    {    
        address = address - FLASH_BASE;
        uint8_t data = this->flash[address];  
        return data;
    }
    else
    {
        fprintf(stderr, "read8(): RAM\n");
        address = address - RAM_BASE;
        uint8_t data = this->ram[address];
        return data;
    }
}

uint16_t Cpu::read16(uint32_t address) const 
{
    if (address < RAM_BASE)
    {    
        address = address - FLASH_BASE;
    }
    else
    {
        address = address - RAM_BASE;
    }

    uint32_t data = this->ram[address] | (this->ram[address + 1] << 8);

    return data;
}   

InstrClass Cpu::classify(uint16_t instr)
{
    if ((instr & 0b1111000000000000) == 0b1101000000000000) 
    {
        if ((instr & 0b0000111100000000) == 0b0000111100000000)
            return InstrClass::MISC;          // 1101 1111 xxxx xxxx
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
        }
    }

    if ((instr & 0b1110000000000000) == 0b0010000000000000) 
    {
        return InstrClass::MOV_CMP_ADD_SUB;  // 001xx xxxx xxxx
    }

    return InstrClass::UNKNOWN;
}

void Cpu::ALUWritePC(uint32_t address)
{
    this->regs[15] = address & ~1u;
}

void Cpu::executeSpecialInstructions()
{
    SpecialInstruction decoded =
        decodedInstruction._SpecialInstruction;

    switch (specialOpType)
    {
        case SpecialOp::ADD:
        {
            fprintf(stderr,
                    "ADD (Register)\n");

            AddHighInstruction i =
                decoded.add;

            const auto result =
                addWithCarry(
                    regs[i.d],
                    regs[i.m],
                    0);

            if (i.d == 15)
            {
                ALUWritePC(
                    result.result);
            }
            else
            {
                regs[i.d] =
                    result.result;

                regs[15] += 2;
            }

            break;
        }

        case SpecialOp::CMP:
        {
            fprintf(stderr,
                    "CMP (Register)\n");

            CmpHighInstruction i =
                decoded.cmp;

            const auto result =
                addWithCarry(
                    regs[i.rn],
                    ~regs[i.rm],
                    1);

            xpsr.setC(
                result.carry_out);

            xpsr.setV(
                result.overflow);

            xpsr.setZ(
                result.result == 0);

            xpsr.setN(
                (result.result >> 31) & 1);

            regs[15] += 2;

            break;
        }

        case SpecialOp::MOV:
        {
            fprintf(stderr,
                    "MOV (Register)\n");

            MovHighInstruction i =
                decoded.mov;

            uint32_t result =
                regs[i.m];

            if (i.d == 15)
            {
                ALUWritePC(result);
            }
            else
            {
                regs[i.d] =
                    result;

                regs[15] += 2;
            }

            break;
        }

        case SpecialOp::BX_BLX:
        {
            BranchExchangeInstruction i =
                decoded.bx;

            uint32_t target =
                regs[i.rm];

            if (i.blx)
            {
                fprintf(stderr,
                        "BLX\n");

                // next instruction
                uint32_t next_instr_addr =
                    regs[15] + 2;

                regs[14] =
                    next_instr_addr | 1u;

                BLXWritePC(target);
            }
            else
            {
                fprintf(stderr,
                        "BX\n");

                BXWritePC(target);
            }

            break;
        }
    }
}


void Cpu::handleSpecialInstructions2(uint16_t instruction, Decoded & decoded)
{
    uint8_t op = (instruction >> 8) & 0b11;

    bool H1 = (instruction >> 7) & 1;
    bool H2 = (instruction >> 6) & 1;

    // full 4-bit registers
    uint8_t m =
        ((instruction >> 3) & 0b111) |
        (H2 << 3);

    uint8_t d =
        (instruction & 0b111) |
        (H1 << 3);

    switch (op)
    {
        case 0b00: // ADD (high register)
        {
            fprintf(stderr, "ADD (Register)\n");

            // SP special case
            if (d == 13 || m == 13)
            {
                std::cerr
                    << "SEE ADD (SP plus register)\n";
            }

            decoded.decodedInstruction
                ._SpecialInstruction
                .add =
                    AddHighInstruction(d, m);

            decoded.specialOpType = SpecialOp::ADD;
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
                return;
            }

            // PC forbidden
            if (n == 15 || m == 15)
            {
                std::cerr
                    << "Unpredictable\n";
                return;
            }

            decoded.decodedInstruction
                ._SpecialInstruction
                .cmp =
                    CmpHighInstruction(
                        n,
                        m);

            decoded.specialOpType =
                SpecialOp::CMP;

            break;
        }

        case 0b10: // MOV (register)
        {
            fprintf(stderr,
                    "MOV (Register)\n");

            decoded.decodedInstruction
                ._SpecialInstruction
                .mov =
                    MovHighInstruction(
                        d,
                        m);

            decoded.specialOpType =
                SpecialOp::MOV;

            break;
        }

        case 0b11: // BX / BLX
        {
            fprintf(stderr,
                    "BX or BLX\n");

            decoded.decodedInstruction
                ._SpecialInstruction
                .bx =
                    BranchExchangeInstruction(
                        m,
                        H1); // bit7 selects BLX

            decoded.specialOpType =
                SpecialOp::BX_BLX;

            break;
        }
    }
}


void Cpu::handleSpecialInstructions(uint16_t instruction)
{
    uint8_t op = (instruction >> 8) & 0b11;

    bool H1 = (instruction >> 7) & 1;
    bool H2 = (instruction >> 6) & 1;

    // full 4-bit registers
    uint8_t m =
        ((instruction >> 3) & 0b111) |
        (H2 << 3);

    uint8_t d =
        (instruction & 0b111) |
        (H1 << 3);

    switch (op)
    {
        case 0b00: // ADD (high register)
        {
            fprintf(stderr, "ADD (Register)\n");

            // SP special case
            if (d == 13 || m == 13)
            {
                std::cerr
                    << "SEE ADD (SP plus register)\n";
            }

            decodedInstruction
                ._SpecialInstruction
                .add =
                    AddHighInstruction(d, m);

            specialOpType = SpecialOp::ADD;
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
                return;
            }

            // PC forbidden
            if (n == 15 || m == 15)
            {
                std::cerr
                    << "Unpredictable\n";
                return;
            }

            decodedInstruction
                ._SpecialInstruction
                .cmp =
                    CmpHighInstruction(
                        n,
                        m);

            specialOpType =
                SpecialOp::CMP;

            break;
        }

        case 0b10: // MOV (register)
        {
            fprintf(stderr,
                    "MOV (Register)\n");

            decodedInstruction
                ._SpecialInstruction
                .mov =
                    MovHighInstruction(
                        d,
                        m);

            specialOpType =
                SpecialOp::MOV;

            break;
        }

        case 0b11: // BX / BLX
        {
            fprintf(stderr,
                    "BX or BLX\n");

            decodedInstruction
                ._SpecialInstruction
                .bx =
                    BranchExchangeInstruction(
                        m,
                        H1); // bit7 selects BLX

            specialOpType =
                SpecialOp::BX_BLX;

            break;
        }
    }
}

uint32_t Cpu::fetch2(uint32_t pc)
{
    uint32_t instr = this->flash[pc] |
                (this->flash[pc + 1] << 8)  |
                (this->flash[pc + 2] << 16) |
                (this->flash[pc + 3] << 24);
    return instr;
}

uint32_t Cpu::fetch(void)
{
    // little endians lsb first
    uint32_t pc = this->regs[15];

    uint32_t instr = this->flash[pc] |
                    (this->flash[pc + 1] << 8)  |
                    (this->flash[pc + 2] << 16) |
                    (this->flash[pc + 3] << 24);

    this->fetched_instruction = instr;

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
                while(1);

                break;
            case SRType_RRX:

                while(1);
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

bool Cpu::is32bitInstruction(uint32_t instruction)
{
    uint8_t thumb_mode = (instruction >> 11) & 0x1F; 

    if (thumb_mode == 0b11101 || thumb_mode == 0b11110 || thumb_mode == 0b11111)
    {
        return true;
    }

    return false;
}

void Cpu::executeLoadStoreHalf(void)
{
    LoadStoreHalfInstruction decoded = this->decodedInstruction.loadStoreHalfInstruction;
    
    uint8_t imm5 = decoded.imm5;
    uint8_t n = decoded.rn;
    uint8_t t = decoded.rt; 

    if (decoded.opcode) 
    {
        uint32_t imm32 = (uint32_t)(imm5 << 2);

        uint32_t Rn = this->regs[n];

        uint32_t offset_addr = Rn + imm32;

        uint32_t address = offset_addr;

        this->regs[t] = read16(address);
    } 
    else 
    {

        
        uint32_t imm32 = (uint32_t)(imm5 << 2);
        uint32_t Rn = this->regs[n];

        uint32_t offset_addr = Rn + imm32;

        uint32_t address = offset_addr;

        print_state();
        fprintf(stderr, "regs[t]: %x regs[n]: %x\n", this->regs[t], this->regs[n]);
        fprintf(stderr, "addr: %x\n", address);
        write16(address, (uint16_t) this->regs[t]);
    }
}

void Cpu::executeLoadStoreImm(void)
{
    LoadStoreImmInstruction decoded = this->decodedInstruction.loadStoreImmInstruction;
    uint8_t imm5 = decoded.imm5;
    uint8_t n = decoded.rn;
    uint8_t t = decoded.rt;

    fprintf(stderr,"Rn = r%d = 0x%08x\n", n, regs[n]);
    fprintf(stderr,"Rt = r%d = 0x%08x\n", t, regs[t]);
    fprintf(stderr,"imm5 = %d\n", imm5);

    switch (decoded.opcode) 
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

            uint32_t value = read32(address);

            fprintf(stderr,"Loaded value = 0x%08x\n", value);

            regs[t] = value;

            break;
        }

        case 0b10: // STRB
        {
            fprintf(stderr,"STRB (IMMEDIATE)\n");
            uint32_t imm32 = imm5;

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
            
            uint32_t imm32 = imm5;

            uint32_t Rn = this->regs[n];

            uint32_t offset_addr = Rn + imm32;

            uint32_t address = offset_addr;

            fprintf(stderr, "LDRB addr=%08x\n", address);
            fprintf(stderr, "byte=%02x\n", read8(address));
            this->regs[t] = read8(address);

            break;
        }
    }
}

void Cpu::handleShiftImmediate(uint16_t instr)
{
    this->decodedInstruction.shiftImmediateInstruction = ShiftImmediateInstruction(instr);
    this->decodeImmShiftResult = decodeImmShift(0b00, this->decodedInstruction.shiftImmediateInstruction.reserved);
}

void Cpu::handleShiftImmediate2(uint16_t instr, Decoded & decoded)
{
    decoded.decodedInstruction.shiftImmediateInstruction = ShiftImmediateInstruction(instr);
    decoded.decodeImmShiftResult = decodeImmShift(0b00, this->decodedInstruction.shiftImmediateInstruction.reserved);
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




void Cpu::BXWritePC(uint32_t address)
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
        this->regs[15] = address & ~1u;
    }
}

uint32_t Cpu::read32(uint32_t address) const
{
    if (address < RAM_BASE)
    {    
        address = address - FLASH_BASE;
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

void Cpu::setPrimaskPM(bool value)
{
    this->primask |= value;
}

void Cpu::executeMisc(void)
{
    switch (misc_type)
    {
        case SEV:
            // Handle SEV
            break;

        case NOP:
            // Handle NOP
            break;

        case SVC:
        {
            // Handle SVC
            printf("SVC\n");
            exceptionPending[11] = true;
            // exceptionTaken(11);
            break;
        }

        case CPS:
            // Handle CPS
            break;

        case SUBADD:
        // Handle SUBADD
        {
            AddSubSpInstruction decoded = this->decodedInstruction._AddSubSpInstruction;
           
            const uint8_t imm7 = decoded.imm7;
            const uint32_t imm32 = ((uint32_t) imm7 << 2);

            if (decoded.bits_7)
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

            regs[15] += 2;
            break;
        }

        case PUSHPOP:
        // Handle PUSHPOP
        {
            PushPopInstruction decoded = this->decodedInstruction._PushPopInstruction;
        
            bool M = decoded.M;    // LR (push) / PC (pop)
            
            if (decoded.op) 
            {
                uint16_t registers = decoded.register_list | (M << 15);

                fprintf(stderr, "POP\n");

                if (std::bitset<16>(registers).count() < 1)
                {
                    return;
                }

                uint32_t address = this->regs[13];

                printf("Address: %x\n", address);
                // Pop r0-r7
                for (uint8_t i = 0; i < 8; i++)
                {
                    if (registers & (1u << i))
                    {
                        this->regs[i] =
                            this->read32(address);

                        address += 4;
                    }
                }

                // Pop PC
                if (M)
                {
                    //printf("BXWRITE PC IN POP\n");
                    //printf("%x\n", address);
                    uint32_t value = this->read32(address);
                    //printf("%d\n", value);
                    this->BXWritePC(this->read32(address));
                }
                else
                {
                    this->regs[15] += 2;
                }

                // Final SP after pops
                this->regs[13] += 4 * std::bitset<16>(registers).count();
            } 
            else 
            {
                uint16_t registers = decoded.register_list | (M << 14);

                fprintf(stderr, "PUSH\n");

                uint32_t count =
                    std::bitset<16>(registers).count();

                uint32_t address = regs[13] - (4 * count);

                // Push in ascending register order
                for (int i = 0; i <= 14; i++)
                {
                    if (registers & (1u << i))
                    {
                        write32(address, regs[i]);
                        address += 4;
                    }
                }

                // Final SP
                regs[13] -= (4 * count);
                printf("Address: %x\n", address);
                regs[15] += 2;
            }

             break;
           }

        // Handle REV
        case REV:
        {
           RevInstruction decoded = this->decodedInstruction._RevInstruction;

            uint8_t m = decoded.rm;
            uint8_t d = decoded.rd;

            // uint32_t v = regs[rm];

            switch (decoded.op) 
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

            regs[15] += 2;
            break;
        }

        case EXTEND:
        {
            ExtendInstruction decoded = this->decodedInstruction._ExtendInstruction;

            uint8_t m = decoded.rm;
            uint8_t d = decoded.rd;

            switch (decoded.op) 
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

            regs[15] += 2;

            break;
        }
        default:
            // Handle unknown type
            break;
    }
}

void Cpu::handleMisc2(uint16_t instr, Decoded & decoded)
{
    fprintf(stderr, "Handle MISC: \n");
    // std::cout << std::hex << instr << std::endl;

    if ((instr & 0xFFFF) == 0b1011111101000000) 
    {
        fprintf(stderr, "SEV\n");
        decoded.misc_type = SEV;
        return;
    }

    if ((instr & 0xFFFF) == 0b1011111100000000) 
    {
        fprintf(stderr, "NOP\n");
      decoded.misc_type = NOP;
        return;
    }

    if ((instr & 0xFFFF) == 0b1101111100000000) 
    {
        fprintf(stderr, "SVC Thumb1Instruction");
         decoded.misc_type = SVC;
        return;
    }

    if ((instr & 0b1111111111100000) == 0b1011011001100000) 
    {
        fprintf(stderr, "CPS\n");
        decoded.misc_type = CPS;
         decoded.decodedInstruction.cPSInstruction = CPSInstruction(instr);
        return;
    }

    // ---- ADD / SUB SP ----
    if ((instr & 0b1111111100000000) == 0b1011000000000000) 
    {
        decoded.misc_type = SUBADD;
        decoded.decodedInstruction._AddSubSpInstruction = AddSubSpInstruction(instr);
        return;
    }

    // // ---- PUSH / POP ----
    if ((instr & 0b1111000000000000) == 0b1011000000000000) 
    {
        decoded.misc_type = PUSHPOP;
        decoded.decodedInstruction._PushPopInstruction = PushPopInstruction(instr);
        return;
    }

    // ---- EXTEND (UXTB/SXTB/UXTH/SXTH) ----
    if ((instr & 0b1111110000000000) == 0b1011001000000000) 
    {
        decoded.misc_type = EXTEND;
        decoded.decodedInstruction._ExtendInstruction = ExtendInstruction(instr);
        return;
    }

    // REV family
    if ((instr & 0b1111110000000000) == 0b1011101000000000) 
    {
        decoded.misc_type = REV;
        decoded.decodedInstruction._RevInstruction = RevInstruction(instr);
        return;
    }
} 

void Cpu::handleMisc(uint16_t instr) 
{
    fprintf(stderr, "Handle MISC: \n");
    // std::cout << std::hex << instr << std::endl;

    if ((instr & 0xFFFF) == 0b1011111101000000) 
    {
        fprintf(stderr, "SEV\n");
        this->misc_type = SEV;
        return;
    }

    if ((instr & 0xFFFF) == 0b1011111100000000) 
    {
        fprintf(stderr, "NOP\n");
        this->misc_type = NOP;
        return;
    }

    if ((instr & 0xFFFF) == 0b1101111100000000) 
    {
        fprintf(stderr, "SVC Thumb1Instruction");
        this->misc_type = SVC;
        return;
    }

    if ((instr & 0b1111111111100000) == 0b1011011001100000) 
    {
        fprintf(stderr, "CPS\n");
        this->misc_type = CPS;
        this->decodedInstruction.cPSInstruction = CPSInstruction(instr);
        return;
    }

    // ---- ADD / SUB SP ----
    if ((instr & 0b1111111100000000) == 0b1011000000000000) 
    {
        this->misc_type = SUBADD;
        this->decodedInstruction._AddSubSpInstruction = AddSubSpInstruction(instr);
        return;
    }

    // // ---- PUSH / POP ----
    if ((instr & 0b1111000000000000) == 0b1011000000000000) 
    {
        this->misc_type = PUSHPOP;
        this->decodedInstruction._PushPopInstruction = PushPopInstruction(instr);
        return;
    }

    // ---- EXTEND (UXTB/SXTB/UXTH/SXTH) ----
    if ((instr & 0b1111110000000000) == 0b1011001000000000) 
    {

        this->misc_type = EXTEND;
        this->decodedInstruction._ExtendInstruction = ExtendInstruction(instr);
        return;
    }

    // REV family
    if ((instr & 0b1111110000000000) == 0b1011101000000000) 
    {
        this->misc_type = REV;
        this->decodedInstruction._RevInstruction = RevInstruction(instr);
        return;
    }
}

void Cpu::executeAdr(void)
{
    AdrInstruction decoded = this->decodedInstruction.adrInstruction;

    uint8_t d   = decoded.rd;
    uint8_t imm8 = decoded.imm8;
    uint32_t imm32 = ((uint32_t) imm8) << 2;

    if (decoded.bits_11) 
    {
        // ADD (SP plust immediate)
        uint32_t SP = this->regs[13];

        auto result = this->addWithCarry(SP, imm32, 0);
        this->regs[d] = result.result;
    } 
    else 
    {
        // ADR
        uint32_t result = (this->regs[15] + 4) & ~0x3;
        this->regs[d] = result;
    } 
}

void Cpu::executeLoadStoreReg(void)
{
    LoadStoreRegInstruction decoded = this->decodedInstruction.loadStoreRegInstruction;
    
    uint8_t m = decoded.rm;
    uint8_t t = decoded.rt;
    uint8_t n = decoded.rn;

    switch (decoded.opcode) 
    {
        case 0b000: // STR
        {
            bool index = true;
            bool add = true;
            bool wback = false;                  
            const auto offset = shift(this->regs[m], SRType_LSL, 0, xpsr.C());
            uint32_t address = this->regs[n] + offset;
            write32(address, this->regs[t]);
            
            break;
        }
        case 0b001: // STRH
        {
            std::cerr << "STRH" << std::endl;

            bool index = true;
            bool add = true;
            bool wback = false;                  
            const auto offset = shift(this->regs[m], SRType_LSL, 0, xpsr.C());
            uint32_t address = this->regs[n] + offset;
            
            write16(address, this->regs[t]);
            
            break;
        }
        case 0b010: // STRB
        {   
            bool index = true;
            bool add = true;
            bool wback = false;                  
            const auto offset = shift(this->regs[m], SRType_LSL, 0, xpsr.C());
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

            const auto offset = shift(this->regs[m], SRType_LSL, 0, xpsr.C());

            uint32_t offset_addr =  add ? Rn + offset : Rn - offset;
            uint32_t address = index ? offset_addr : Rn;

            uint8_t data = this->read8(address);

            this->regs[t] = sign_extend(data, 32); 
            break;   
        }
        case 0b100: // LDR
        {
            fprintf(stderr, "LDR (Register)\n");
            uint32_t Rn = this->regs[n];

            const auto offset = shift(this->regs[m], SRType_LSL, 0, xpsr.C());

            uint32_t offset_addr = Rn + offset;
            uint32_t address = offset_addr;

            this->regs[t] = this->read32(address);
            break;
        }
        case 0b101: // LDRH
        {
            uint32_t Rn = this->regs[n];

            const auto offset = shift(this->regs[m], SRType_LSL, 0, xpsr.C());

            uint32_t offset_addr =  Rn + offset;
            uint32_t address = offset_addr;

            this->regs[t] = (uint32_t) this->read16(address);
    
            break;
        }
        case 0b110: // LDRB
        {
            uint32_t Rn = this->regs[n];

            const auto offset = shift(this->regs[m], SRType_LSL, 0, xpsr.C());

            uint32_t offset_addr = Rn + offset;
            uint32_t address = offset_addr;

            this->regs[t] = (uint32_t) this->read8(address);
    
            break;
        }
        case 0b111: // LDRSH
        {
            uint32_t Rn = this->regs[n];

            const auto offset = shift(this->regs[m], SRType_LSL, 0, xpsr.C());

            uint32_t offset_addr = Rn + offset;
            uint32_t address = offset_addr;

            this->regs[t] = sign_extend(this->read16(address), 32);

            break;
        }
    }

}

void Cpu::executeSpRelative(void)
{
    SpRelativeInstruction decoded = this->decodedInstruction.SpRelativeInstruction;

    uint8_t t   = decoded.rt;
    uint8_t imm8 = decoded.imm8;
    uint8_t n = 13;
    uint32_t imm32 = (uint32_t)(imm8 << 2); 
    uint32_t Rn = this->regs[n];
    
    if (decoded.opcode) 
    {
        uint32_t offset_addr = Rn + imm32;
        this->regs[t] = read32(offset_addr);
    } 
    else 
    {
        uint32_t offset_addr = Rn + imm32;
        write32(offset_addr, this->regs[t]);
    }
}

void Cpu::executeMultiple()
{
    LoadMultipleInstruction decoded = this->decodedInstruction._LoadMultipleInstruction;

    uint8_t n = decoded.rn;
    uint8_t register_list = decoded.register_list;

    if (decoded.op) 
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
                    write32(address, 0x0);
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


// void Cpu::executeCondBranch()
// {
//     BranchCondInstruction decoded = this->decodedInstruction._BranchCondInstruction;

//     uint8_t cond = decoded.cond;
//     uint8_t imm8 = decoded.imm8;

//     if (cond == 0b1110)
//     {

//     }
//     else if (cond == 0b1111)
//     {

//     }

//     int32_t imm32 = sign_extend(imm8 << 1, 9);

//     if (conditionPassed(cond))
//     {
//         regs[15] = (regs[15] + 4 + imm32) & ~1;
//     }
//     else
//     {
//         this->regs[15] += 2;
//     }
// };

void Cpu::executeCondBranch()
{
    BranchCondInstruction decoded = this->decodedInstruction._BranchCondInstruction;

    uint8_t cond = decoded.cond;
    uint8_t imm8 = decoded.imm8;

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
};

bool Cpu::currentModeIsPrivileged(void)
{
    return (this->currentMode == Mode::MODE_HANDLER || (this->control.nPRIV & 0b1) == 0); 
}

void Cpu::executeUncondBranch()
{
    BranchUncondInstruction decoded = this->decodedInstruction._BranchUncondInstruction;

    int32_t imm32 = sign_extend(decoded.imm11 << 1, 12);

    uint32_t next_pc = this->regs[15] + 4;

    this->regs[15] = next_pc + imm32;
}

void Cpu::executeShiftImmediate(void)
{
    fprintf(stderr,"LSL (Immediate)\n");
    ShiftImmediateInstruction decoded = this->decodedInstruction.shiftImmediateInstruction;
    DecodeImmShiftResult result = this->decodeImmShiftResult;
    auto shifted = shift_c(this->regs[decoded.rm], SRType_LSL, result.n, xpsr.C());
    this->regs[decoded.rd] = shifted.result;
    

    
    this->xpsr.setC(shifted.carry_out);
    this->xpsr.setZ(shifted.result == 0);
    this->xpsr.setN((shifted.result >> 31) & 0b1);


    this->regs[15] += 2;
}

void Cpu::setAPSRValues(bool c, bool n, bool v, bool z)
{
    this->xpsr.setC(c);
    this->xpsr.setN(n);
    this->xpsr.setV(v);
    this->xpsr.setZ(z);
}

void Cpu::executeALUinstr(void)
{
    ALUInstruction decoded = this->decodedInstruction._ALUInstruction;

    uint8_t n = decoded.rdn;
    uint8_t d = decoded.rdn;
    uint8_t m = decoded.rm;

    switch (decoded.opcode)
    {
        case 0x0: // AND
        {
            fprintf(stderr, "AND\n");
            uint32_t Rm = this->regs[m];

            uint8_t shift_n = 0;
            SRType type = SRType_LSL;    
            Shift_c shifted = this->shift_c(Rm, type, shift_n, this->xpsr.C());                              

            uint32_t result = shifted.result & this->regs[n];

            this->regs[d] = result;
            this->xpsr.setC(shifted.carry_out);
            this->xpsr.setZ(shifted.result == 0);
            this->xpsr.setN((shifted.result >> 31) & 0b1);
                    
            break;
        }

        case 0x1: // EOR
        {
            fprintf(stderr, "EOR\n");

            uint32_t Rm = this->regs[m];
            uint32_t Rn = this->regs[n];

            Shift_c shifted = this->shift_c(Rm, SRType_LSL, 0, xpsr.C());

            uint32_t result = Rn ^ shifted.result;

            this->regs[d] = result;
            this->xpsr.setC(shifted.carry_out);
            this->xpsr.setZ(shifted.result == 0);
            this->xpsr.setN((shifted.result >> 31) & 0b1);
                    
            
            break;
        }

        case 0x2: // LSL (register)
        {
            fprintf(stderr, "LSL (register)\n");
            const uint8_t shift_n = 0;

            const SRType type = SRType_LSL;

            const uint32_t Rm = this->regs[m];
            const uint32_t Rn = this->regs[n];
            const uint32_t shifted = this->shift(Rm, type, shift_n, this->xpsr.C());

            const AddCarryResult carry_result = this->addWithCarry(Rn,shifted, this->xpsr.C());

            this->regs[d] = carry_result.result;

            this->xpsr.setC(carry_result.carry_out);
            this->xpsr.setZ(carry_result.result == 0);
            this->xpsr.setN((carry_result.result >> 31) & 0b1);
            this->xpsr.setV(carry_result.overflow);
                    
            break;
        }

        case 0x3: // LSR (register)
        {
            fprintf(stderr, "LSR (register)\n");
            const uint8_t shift_n = this->regs[m];

            const auto shifted = this->shift_c(this->regs[n], SRType_LSR, shift_n, this->xpsr.C());

            this->regs[d] = shifted.result;

            this->xpsr.setC(shifted.carry_out);
            this->xpsr.setZ(shifted.result == 0);
            this->xpsr.setN((shifted.result >> 31) & 0b1);
        
            break;
        }

        case 0x4: // ASR (register)
        {
            fprintf(stderr, "ASR (register)\n");
            const uint8_t shift_n = this->regs[m];

            const auto shifted = this->shift_c(this->regs[n], SRType_ASR, shift_n, this->xpsr.C());

            this->regs[d] = shifted.result;
            
            this->xpsr.setC(shifted.carry_out);
            this->xpsr.setZ(shifted.result == 0);
            this->xpsr.setN((shifted.result >> 31) & 0b1);
            break;
        }

        case 0x5: // ADC
        {
            fprintf(stderr, "ADC\n");
            const auto shifted = this->shift(this->regs[m], SRType_LSL, 0, this->xpsr.C());
            const auto result = addWithCarry(this->regs[n], shifted, xpsr.C());

            this->regs[d] = result.result;

            this->xpsr.setC(result.carry_out);
            this->xpsr.setZ(result.result == 0);
            this->xpsr.setN((result.result >> 31) & 0b1);
            this->xpsr.setV(result.overflow);

            break;
        }
        case 0x6: // SBC
        {
            fprintf(stderr, "SBC\n");
            const auto shifted = this->shift(this->regs[m], SRType_LSL, 0, xpsr.C());

            const auto result = addWithCarry(this->regs[n], ~shifted , xpsr.C());

            this->regs[d] = result.result;

            this->xpsr.setC(result.carry_out);
            this->xpsr.setZ(result.result == 0);
            this->xpsr.setN((result.result >> 31) & 0b1);
            this->xpsr.setV(result.overflow);

            break;
        }

        case 0x7: // ROR
        {
            fprintf(stderr, "ROR\n");
            uint8_t shift_n = this->regs[m] & 0xFF;

            auto result = shift_c(this->regs[n], SRType_ROR, shift_n, xpsr.C());

            this->regs[d] = result.result;


            setAPSRValues(result.carry_out, (result.result >> 31) & 0b1, xpsr.V(), result.result == 0);
            break;
        }

        case 0x8: // TST (no write)
        {
            fprintf(stderr, "TST\n");
            const auto shifted = this->shift_c(this->regs[m], SRType_LSL, 0, xpsr.C());     

            const uint32_t result = this->regs[n] & shifted.result;

            setAPSRValues(shifted.carry_out, (result >> 31) & 0b1, xpsr.V(), result == 0);
            break;
        }

        case 0x9: // NEG
        {
            fprintf(stderr, "NEG\n");

            auto result = addWithCarry(~(this->regs[n]), 0, true);

            this->regs[d] = result.result;

            setAPSRValues(result.carry_out, (result.result >> 31) & 0b1, result.overflow, result.result == 0);
            break;
        }

        case 0xA: // CMP (no write)
        {
            fprintf(stderr, "CMP (Register)\n");
            const uint32_t shifted = this->shift(this->regs[m], SRType_LSL, 0, this->xpsr.C()); 
            const auto result = addWithCarry(this->regs[n], ~shifted, 1);
            
            setAPSRValues(result.carry_out, (result.result >> 31) & 0b1, result.overflow, result.result == 0);          
            break;
        }

        case 0xB: // CMN (no write)
        {
            fprintf(stderr, "CMN (Register)\n");

            const uint32_t shifted = this->shift(this->regs[m], SRType_LSL, 0, this->xpsr.C()); 

            const auto result = addWithCarry(this->regs[n], shifted, 0);
            setAPSRValues(result.carry_out, (result.result >> 31) & 0b1, result.overflow, result.result == 0);

            break;
        }

        case 0xC: // ORR
        {
            fprintf(stderr, "ORR (Register)\n");
            const Shift_c shifted = this->shift_c(this->regs[m], SRType_LSL, 0, xpsr.C());

            const uint32_t result = this->regs[n] | shifted.result;

            this->regs[d] = result;

            setAPSRValues(shifted.carry_out, (result >> 31) & 0b1, xpsr.V(), result == 0);
            break;
        }
        case 0xD: // MUL
        {
            fprintf(stderr, "MUL\n");
            uint32_t operand1 = this->regs[n];
            uint32_t operand2 = this->regs[m];

            uint64_t result = operand2 * operand1;

            this->regs[d] = (uint32_t) result;

            setAPSRValues(xpsr.C(), (result >> 31) & 0b1, xpsr.V(), result == 0);
            break;
        }

        case 0xE: // BIC
        {
            fprintf(stderr, "BIC\n");
    
            Shift_c shifted = this->shift_c(this->regs[m], SRType_LSL, 0, this->xpsr.C());                              

            uint32_t result = shifted.result & ~(this->regs[n]);

            this->regs[d] = result;

            setAPSRValues(shifted.carry_out, (shifted.result >> 31) & 0b1, xpsr.V(), shifted.result == 0);
        }

        case 0xF: // MVN
        {
            fprintf(stderr, "MVN");
            Shift_c shifted = this->shift_c(this->regs[m], SRType_LSL, 0, this->xpsr.C());                              

            uint32_t result = ~(shifted.result);

            this->regs[d] = result;

            setAPSRValues(shifted.carry_out, (shifted.result >> 31) & 0b1, xpsr.V(), shifted.result == 0);
            break;
        }
    }
}


void Cpu::executeMovCmpAddSub2(const MovCmpAddSubInstruction & decoded)
{
    switch (decoded.op)
    {
        case 0b00: // MOVS Rd, #imm
        {
            fprintf(stderr, "MOV (imm)\n");
            uint32_t imm32 = (uint32_t) decoded.imm8;
            this->regs[decoded.d] = imm32;

            setAPSRValues(xpsr.C(), ((imm32 >> 31) & 0b1), xpsr.V(), imm32 == 0);

            break;
        }

        case 0b01: // CMP Rd, #imm
        {

            fprintf(stderr, "CMP (imm)\n");
            uint32_t imm32 = (uint32_t) decoded.imm8;
            // d == n here
            const auto result = addWithCarry(this->regs[decoded.d], ~imm32, 1);

            setAPSRValues(result.carry_out, (result.result >> 31) & 0b1, result.overflow, imm32 == 0);
 
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

void Cpu::executeMovCmpAddSub(void)
{
    MovCmpAddSubInstruction decoded = this->decodedInstruction.movCmpAddSubInstruction;

    switch (decoded.op)
    {
        case 0b00: // MOVS Rd, #imm
        {
            fprintf(stderr, "MOV (imm)\n");
            uint32_t imm32 = (uint32_t) decoded.imm8;
            this->regs[decoded.d] = imm32;

            setAPSRValues(xpsr.C(), ((imm32 >> 31) & 0b1), xpsr.V(), imm32 == 0);

            break;
        }

        case 0b01: // CMP Rd, #imm
        {

            fprintf(stderr, "CMP (imm)\n");
            uint32_t imm32 = (uint32_t) decoded.imm8;
            // d == n here
            const auto result = addWithCarry(this->regs[decoded.d], ~imm32, 1);

            setAPSRValues(result.carry_out, (result.result >> 31) & 0b1, result.overflow, imm32 == 0);
 
            break;
        }

        case 0b10: // ADDS Rd, #imm
        {
            fprintf(stderr, "ADD (imm)\n");
            uint32_t imm32 = (uint32_t) decoded.imm8;
            // d == n here
            const auto result = addWithCarry(this->regs[decoded.d], imm32, 1);

            setAPSRValues(result.carry_out, (result.result >> 31) & 0b1, result.overflow, imm32 == 0);
            break;
        }

        case 0b11: // SUBS Rd, #imm
        {


            break;
        }
    }
}

void Cpu::executeLDRLiteral(void)
{
    fprintf(stderr, "LDR Literal\n");
    LDRLiteralInstruction decoded = this->decodedInstruction.lDRLiteralInstruction;

    uint32_t imm32 = (uint32_t)(decoded.imm8 << 2); 
    uint32_t base = (this->regs[15] + 4) & ~0x3;
    uint32_t address = base + imm32;

    fprintf(stderr,"imm: %d\n", imm32);
    fprintf(stderr,"dest reg: %x\n", decoded.rt);
    fprintf(stderr,"Address %x\n", imm32 + base);

    this->regs[decoded.rt] = read32(address);
}

void Cpu::executeAddSub(void)
{
    AddSubImmediateInstruction decoded = this->decodedInstruction._AddSubImmediateInstruction;

    uint8_t I  = decoded.bits_10;
    uint8_t Op = decoded.bits_9;

    uint8_t Rn = decoded.rn;
    uint8_t Rs = decoded.imm3;
    uint8_t Rd = decoded.rd;

    fprintf(stderr, "Rd: %d\n", Rd);
    fprintf(stderr, "Rn: %d\n", Rn);
    fprintf(stderr, "imm3: %d\n", Rs);


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
        fprintf(stderr, "ADD\n");
        auto res = addWithCarry(regs[Rn], operand2, 0);

        result = res.result;

        setAPSRValues(res.carry_out, (result >> 31) & 1,res.overflow, (result == 0));
    }
    else
    {
        fprintf(stderr, "SUB\n");
        // SUB
        auto res = addWithCarry(regs[Rn], ~operand2, 1);

        result = res.result;
        setAPSRValues(res.carry_out, (result >> 31) & 1,res.overflow, (result == 0));
    }

    regs[Rd] = result;
    regs[15] += 2;
}

bool Cpu::execute2(Decoded decoded, uint32_t pc)
{
    bool branchTaken = false;

    if (decoded.instructionType == THUMB2)
    {   
        fprintf(stderr, "executing 32 bit instruction\n");

        switch(decoded.thumb2Class)
        {
            case BRANCH_MISC:
            {
                switch(decoded.branch_misc_type)
                {
                    case MRS_Instruction:
                    {

                        MRS inst = decoded.decodedThumb2Instruction._MRSInstruction;
                        uint8_t sysm = inst.sysm;
                        uint8_t d = inst.rd;

                        switch ((sysm >> 3) & 0x1F)
                        {
                            case 0:
                            {
                                // APSR
                                if ((sysm & 0b1) == 1)
                                {
                                    regs[d] = this->xpsr.apsr() & 0xFF;
                                }
                                else if ((sysm & (0b1 << 1)) == 1)
                                {
                                    regs[d] &= ~(1 << 24);
                                }
                                else if ((sysm & (0b1 << 2)) == 0)
                                {
                                    regs[d] = this->xpsr.apsr() & 0xF8000000;
                                }
                                break;
                            }

                            case 1:
                            {
                                if (this->currentModeIsPrivileged())
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
                                }
                                break;
                            }

                            case 2:
                            {
                                switch (sysm & 0x7)
                                {
                                    case 0:
                                        regs[d] &= ~(currentModeIsPrivileged() ? (primask & 0b1) : 0);
                                        break;

                                    case 4:
                                        regs[d] &= ~(control.nPRIV & 0b11);
                                        break;
                                }
                                break;
                            }
                        }
                        break;
                    }
                    case MSR_Instruction:
                    {
                        MSR inst = decoded.decodedThumb2Instruction._MSRInstruction;
                        uint8_t sysm = inst.sysm;
                        uint8_t n = inst.rn;

                        uint32_t Rn = this->regs[n];

                        switch ((sysm >> 3) & 0x1F)
                        {
                            //
                            // APSR
                            //
                            case 0b00000:
                            {
                                if (((sysm >> 2) & 1) == 0)
                                {
                                    // APSR<31:27> = R[n]<31:27>
                                    this->xpsr.setNZCVQ(Rn >> 27);
                                }

                                break;
                            }

                            //
                            // MSP / PSP
                            //
                            case 0b00001:
                            {
                                if (currentModeIsPrivileged())
                                {
                                    switch (sysm & 0x7)
                                    {
                                        //
                                        // MSP
                                        //
                                        case 0b000:
                                        {
                                            this->msp = Rn & ~0x3u;
                                            break;
                                        }

                                        //
                                        // PSP
                                        //
                                        case 0b001:
                                        {
                                            this->psp = Rn & ~0x3u;
                                            break;
                                        }
                                    }
                                }
                            }
                            case 0b00010:
                            {
                                if (currentModeIsPrivileged())
                                {
                                    switch (sysm & 0x7)
                                    {
                                        //
                                        // PRIMASK
                                        //
                                        case 0b000:
                                        {
                                            // FIX ME

                                            std::cout << "FIX ME\n";
                                            // this->primask = Rn & 1u;
                                            break;
                                        }

                                        //
                                        // CONTROL
                                        //
                                        case 0b100:
                                        {
                                            control.nPRIV = Rn & 1u;

                                            if (currentMode == Mode::MODE_THREAD)
                                            {
                                                control.SPSEL =
                                                    (Rn >> 1) & 1u;
                                            }

                                            break;
                                        }
                                    }
                                }
                            }
                        }
                        break;
                    }
                    case UDF:
                    {
                        break;
                    }
                    case BL:
                    {
                        BranchLinkInstruction inst = decoded.decodedThumb2Instruction._BranchLinkInstruction;
                    
                        bool I1 = !(inst.J1 ^ inst.S);
                        bool I2 = !(inst.J2 ^ inst.S);

                        uint32_t imm25 =
                            (inst.S  << 24) |
                            (I1 << 23) |
                            (I2 << 22) |
                            (inst.imm10 << 12) |
                            (inst.imm11 << 1);

                        int32_t  imm32 = sign_extend(imm25, 25);

                        uint32_t next = regs[15] + 4;
                        fprintf(stderr,"BRANCH AND LINK: next pc: %d\n", next);
                
                                fprintf(stderr,"BRANCH AND LINK\n imm32: %d\n", imm32);

                        regs[14] = next | 1;

                        regs[15] = next + imm32;

                        fprintf(stderr,"BRANCH AND LINK\n regs[15: %d\n", regs[15]);
                        break;   
                    }
                    default:
                        break;

                }
                // 1111 1000 0001 0000       1111 0000 0000 000
                break;
            }


            default:
                break;
            // case 0b11101:

            // BL
            // case 0b11110:
            // {


            // case 0b11111:
            //     // 32 bit thumb mode
            //     // 
            //     break;
        }
    }
    else
    {  
        fprintf(stderr, "Executing 16bit\n");
         switch (decoded.instructionClass)
        {
            case InstrClass::SHIFT_IMM:
                executeShiftImmediate();
                std::cerr << "SHIFT_IMM\n";
                break;

            case InstrClass::ADD_SUB:
                executeAddSub();
                std::cerr << "ADD_SUB\n";
                break;

            case InstrClass::MOV_CMP_ADD_SUB:
                executeMovCmpAddSub2(decoded.decodedInstruction.movCmpAddSubInstruction);
                std::cerr << "MOV_CMP_ADD_SUB\n";
                this->regs[15] += 2;
                break;

            case InstrClass::ALU:
                executeALUinstr();
                this->regs[15] += 2;
                std::cerr << "ALU\n";
                break;

            case InstrClass::HI_REG:
                executeSpecialInstructions();
                std::cerr << "HI_REG\n";
                break;

            case InstrClass::LDR_LITERAL:
                std::cerr << "LDR_LITERAL\n";
                executeLDRLiteral();
                this->regs[15] += 2;
                break;

            case InstrClass::LOAD_STORE_REG:
                executeLoadStoreReg();
                std::cerr << "LOAD_STORE_REG\n";
                this->regs[15] += 2;
                break;

            case InstrClass::LOAD_STORE_IMM:
                executeLoadStoreImm();
                this->regs[15] += 2;
                std::cerr << "LOAD_STORE_IMM\n";
                break;

            case InstrClass::LOAD_STORE_HALF:
                executeLoadStoreHalf();
                this->regs[15] += 2;
                std::cerr << "LOAD_STORE_HALF\n";
                break;

            case InstrClass::SP_REL:
                executeSpRelative();
                this->regs[15] += 2;
                break;

            case InstrClass::ADDR:
                std::cerr << "ADDR\n";
                executeAdr();
                this->regs[15] += 2;
                break;

            case InstrClass::MISC:
                std::cerr << "MISC\n";
                executeMisc();
                // this->regs[15] += 2;
                break;

            case InstrClass::MULTIPLE:
                executeMultiple();
                this->regs[15] += 2;
                std::cerr << "MULTIPLE\n";
                break;

            case InstrClass::COND_BRANCH:
                executeCondBranch();
                std::cerr << "COND_BRANCH\n";
                break;

            case InstrClass::UNCOND_BRANCH:
                executeUncondBranch();
                std::cerr << "UNCOND_BRANCH\n";
                break;

            case InstrClass::UNKNOWN:
            default:
                std::cerr << "UNKNOWN\n";
                break;
        }     
    }

    return branchTaken;
}

void Cpu::execute(void)
{
    if (this->instructionType == THUMB2)
    {   
        fprintf(stderr, "executing 32 bit instruction\n");

        switch(this->thumb2Class)
        {
            case BRANCH_MISC:
            {
                switch(this->branch_misc_type)
                {
                    case MRS_Instruction:
                    {

                        MRS decoded = this->decodedThumb2Instruction._MRSInstruction;
                        uint8_t sysm = decoded.sysm;
                        uint8_t d = decoded.rd;

                        switch ((sysm >> 3) & 0x1F)
                        {
                            case 0:
                            {
                                // APSR
                                if ((sysm & 0b1) == 1)
                                {
                                    regs[d] = this->xpsr.apsr() & 0xFF;
                                }
                                else if ((sysm & (0b1 << 1)) == 1)
                                {
                                    regs[d] &= ~(1 << 24);
                                }
                                else if ((sysm & (0b1 << 2)) == 0)
                                {
                                    regs[d] = this->xpsr.apsr() & 0xF8000000;
                                }
                                break;
                            }

                            case 1:
                            {
                                if (this->currentModeIsPrivileged())
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
                                }
                                break;
                            }

                            case 2:
                            {
                                switch (sysm & 0x7)
                                {
                                    case 0:
                                        regs[d] &= ~(currentModeIsPrivileged() ? (primask & 0b1) : 0);
                                        break;

                                    case 4:
                                        regs[d] &= ~(control.nPRIV & 0b11);
                                        break;
                                }
                                break;
                            }
                        }
                        break;
                    }
                    case MSR_Instruction:
                    {
                        MSR decoded = this->decodedThumb2Instruction._MSRInstruction;
                        uint8_t sysm = decoded.sysm;
                        uint8_t n = decoded.rn;

                        uint32_t Rn = this->regs[n];

                        switch ((sysm >> 3) & 0x1F)
                        {
                            //
                            // APSR
                            //
                            case 0b00000:
                            {
                                if (((sysm >> 2) & 1) == 0)
                                {
                                    // APSR<31:27> = R[n]<31:27>
                                    this->xpsr.setNZCVQ(Rn >> 27);
                                }

                                break;
                            }

                            //
                            // MSP / PSP
                            //
                            case 0b00001:
                            {
                                if (currentModeIsPrivileged())
                                {
                                    switch (sysm & 0x7)
                                    {
                                        //
                                        // MSP
                                        //
                                        case 0b000:
                                        {
                                            this->msp = Rn & ~0x3u;
                                            break;
                                        }

                                        //
                                        // PSP
                                        //
                                        case 0b001:
                                        {
                                            this->psp = Rn & ~0x3u;
                                            break;
                                        }
                                    }
                                }
                            }
                            case 0b00010:
                            {
                                if (currentModeIsPrivileged())
                                {
                                    switch (sysm & 0x7)
                                    {
                                        //
                                        // PRIMASK
                                        //
                                        case 0b000:
                                        {
                                            // FIX ME

                                            std::cout << "FIX ME\n";
                                            // this->primask = Rn & 1u;
                                            break;
                                        }

                                        //
                                        // CONTROL
                                        //
                                        case 0b100:
                                        {
                                            control.nPRIV = Rn & 1u;

                                            if (currentMode == Mode::MODE_THREAD)
                                            {
                                                control.SPSEL =
                                                    (Rn >> 1) & 1u;
                                            }

                                            break;
                                        }
                                    }
                                }
                            }
                        }
                        break;
                    }
                    case UDF:
                    {
                        break;
                    }
                    case BL:
                    {
                        BranchLinkInstruction decoded = this->decodedThumb2Instruction._BranchLinkInstruction;
                    
                        bool I1 = !(decoded.J1 ^ decoded.S);
                        bool I2 = !(decoded.J2 ^ decoded.S);

                        uint32_t imm25 =
                            (decoded.S  << 24) |
                            (I1 << 23) |
                            (I2 << 22) |
                            (decoded.imm10 << 12) |
                            (decoded.imm11 << 1);

                        int32_t  imm32 = sign_extend(imm25, 25);

                        uint32_t next = regs[15] + 4;
                        fprintf(stderr,"BRANCH AND LINK: next pc: %d\n", next);
                
                                fprintf(stderr,"BRANCH AND LINK\n imm32: %d\n", imm32);

                        regs[14] = next | 1;

                        regs[15] = next + imm32;

                        fprintf(stderr,"BRANCH AND LINK\n regs[15: %d\n", regs[15]);
                        break;   
                    }
                    default:
                        break;

                }
                // 1111 1000 0001 0000       1111 0000 0000 000
                break;
            }


            default:
                break;
            // case 0b11101:

            // BL
            // case 0b11110:
            // {


            // case 0b11111:
            //     // 32 bit thumb mode
            //     // 
            //     break;
        }
    }
    else
     {  
        fprintf(stderr, "Executing 16bit\n");
         switch (this->instructionClass)
        {
            case InstrClass::SHIFT_IMM:
                executeShiftImmediate();
                std::cerr << "SHIFT_IMM\n";
                break;

            case InstrClass::ADD_SUB:
                executeAddSub();
                std::cerr << "ADD_SUB\n";
                break;

            case InstrClass::MOV_CMP_ADD_SUB:
                executeMovCmpAddSub();
                std::cerr << "MOV_CMP_ADD_SUB\n";
                this->regs[15] += 2;
                break;

            case InstrClass::ALU:
                executeALUinstr();
                this->regs[15] += 2;
                std::cerr << "ALU\n";
                break;

            case InstrClass::HI_REG:
                executeSpecialInstructions();
                std::cerr << "HI_REG\n";
                break;

            case InstrClass::LDR_LITERAL:
                std::cerr << "LDR_LITERAL\n";
                executeLDRLiteral();
                this->regs[15] += 2;
                break;

            case InstrClass::LOAD_STORE_REG:
                executeLoadStoreReg();
                std::cerr << "LOAD_STORE_REG\n";
                this->regs[15] += 2;
                break;

            case InstrClass::LOAD_STORE_IMM:
                executeLoadStoreImm();
                this->regs[15] += 2;
                std::cerr << "LOAD_STORE_IMM\n";
                break;

            case InstrClass::LOAD_STORE_HALF:
                executeLoadStoreHalf();
                this->regs[15] += 2;
                std::cerr << "LOAD_STORE_HALF\n";
                break;

            case InstrClass::SP_REL:
                executeSpRelative();
                this->regs[15] += 2;
                break;

            case InstrClass::ADDR:
                std::cerr << "ADDR\n";
                executeAdr();
                this->regs[15] += 2;
                break;

            case InstrClass::MISC:
                std::cerr << "MISC\n";
                executeMisc();
                // this->regs[15] += 2;
                break;

            case InstrClass::MULTIPLE:
                executeMultiple();
                this->regs[15] += 2;
                std::cerr << "MULTIPLE\n";
                break;

            case InstrClass::COND_BRANCH:
                executeCondBranch();
                std::cerr << "COND_BRANCH\n";
                break;

            case InstrClass::UNCOND_BRANCH:
                executeUncondBranch();
                std::cerr << "UNCOND_BRANCH\n";
                break;

            case InstrClass::UNKNOWN:
            default:
                std::cerr << "UNKNOWN\n";
                break;
        }     
    }
}

#include <unordered_map>

Decoded Cpu::decode2(uint32_t instruction)
{
    Decoded decoded = {};

    uint8_t format_id = (instruction >> 11) & 0x1F;   

    std::unordered_map<uint8_t, uint32_t> registers_read;


    if (is32bitInstruction(instruction))
    {
        decoded.instructionType = THUMB2;
    
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

                    decoded.decodedThumb2Instruction._MSRInstruction = MSR(n, sysm);
                    decoded.thumb2Class = BRANCH_MISC;
                    decoded.branch_misc_type = MSR_Instruction;
                
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
                        return Decoded { };
                    }

                    switch (op)
                    {
                        case 0b0100:
                            decoded.decodedThumb2Instruction._DSBInstruction = DSB(option);
                            decoded.thumb2Class = BRANCH_MISC;
                            decoded.branch_misc_type = BL;

                            break;

                        case 0b0101:
                            decoded.decodedThumb2Instruction._DMBInstruction = DMB(option);
                            decoded.thumb2Class = BRANCH_MISC;
                            decoded.branch_misc_type = BL;
                            break;

                        case 0b0110:
                            decoded.decodedThumb2Instruction._ISBInstruction = ISB(option);
                            break;

                        default:
                            break;
                    }
                }
                // MRS
                else if ((op1 & 0b1111110) == 0b0111110 &&  ((op2 & 0b001) | (op2 & 0b100)) == 0b000)
                {
                    decoded.decodedThumb2Instruction._MRSInstruction = MRS(second & 0x7, first & 0xFF);
                    decoded.thumb2Class = BRANCH_MISC;
                    decoded.branch_misc_type = MRS_Instruction;
                                        // uint8_t d = second & 0x7;
                    // uint8_t sysm = first & 0xFF;         
                }
                else if ((op1 & 0b1111111) == 0b1111111 && op2 == 0b010)
                {
                    // undefined
                }
                else if (((op2 & 0b001) | (op2 & 0b100)) == 0b101)
                {
                    fprintf(stderr,"BRANCH AND LINK\n");
                    decoded.decodedThumb2Instruction._BranchLinkInstruction = BranchLinkInstruction(instruction);
                    decoded.thumb2Class = BRANCH_MISC;
                    decoded.branch_misc_type = BL;
                }
                break;
            }
            case 0b11111:
            {
                break;
            }
        }
    }
    else
    {
        decoded.instructionType = THUMB1;
        decoded.instructionClass = classify(instruction);
        std::cerr << "Thumb1Instruction:" << std::bitset<16>((uint16_t)instruction) << std::endl;
        std::cerr << "Class: " << static_cast<int>( decoded.instructionClass) << std::endl;

        switch ( decoded.instructionClass)
        {
            case InstrClass::SHIFT_IMM:
                handleShiftImmediate2(instruction, decoded);
                break;

            case InstrClass::ADD_SUB:
                decoded.decodedInstruction._AddSubImmediateInstruction =
                    AddSubImmediateInstruction(instruction);
                break;

            case InstrClass::MOV_CMP_ADD_SUB:
                            decoded.decodedInstruction.movCmpAddSubInstruction =
                    MovCmpAddSubInstruction(instruction);
            // {
            //     uint32_t imm32 = instruction & 0xFF;
            //     uint8_t d = (instruction >> 8) & 0x7;
                

            //     // op = blah blah
            //     registers_read.emplace(d, regs[d]);

            //     break;
            // }

                break;

            case InstrClass::ALU:
                decoded.decodedInstruction._ALUInstruction =
                    ALUInstruction(instruction);
                break;

            case InstrClass::HI_REG:
                this->handleSpecialInstructions2(instruction, decoded);
                break;

            case InstrClass::LDR_LITERAL:
                decoded.decodedInstruction.lDRLiteralInstruction = LDRLiteralInstruction(instruction);
                break;

            case InstrClass::LOAD_STORE_REG:
                decoded.decodedInstruction.loadStoreRegInstruction = LoadStoreRegInstruction(instruction);
                break;

            case InstrClass::LOAD_STORE_IMM:
                decoded.decodedInstruction.loadStoreImmInstruction = 
                    LoadStoreImmInstruction(instruction);
                break;

            case InstrClass::LOAD_STORE_HALF:
                decoded.decodedInstruction.loadStoreHalfInstruction = 
                    LoadStoreHalfInstruction(instruction);
                break;

            case InstrClass::SP_REL:
                decoded.decodedInstruction.SpRelativeInstruction = 
                    SpRelativeInstruction(instruction);
                break;

            case InstrClass::ADDR:
                decoded.decodedInstruction.adrInstruction = 
                    AdrInstruction(instruction);
                break;

            case InstrClass::MISC:
                handleMisc2(instruction, decoded);
                break;

            case InstrClass::MULTIPLE:
                decoded.decodedInstruction._LoadMultipleInstruction = 
                    LoadMultipleInstruction(instruction);
                break;

            case InstrClass::COND_BRANCH:
                decoded.decodedInstruction._BranchCondInstruction =
                    BranchCondInstruction(instruction);
                break;

            case InstrClass::UNCOND_BRANCH:
                decoded.decodedInstruction._BranchUncondInstruction =
                    BranchUncondInstruction(instruction);
                break;

            case InstrClass::UNKNOWN:
            default:
                std::cerr << "UNKNOWN\n";
                break;
        }     
    }

    return decoded;
}


void Cpu::decode(void)
{
    uint32_t instruction = this->fetched_instruction;
    uint8_t format_id = (instruction >> 11) & 0x1F;   

    std::cerr << "PC" << this->regs[15] << std::endl;

    if (is32bitInstruction(format_id))
    {
        this->instructionType = THUMB2;
        this->decodedInstructionSize = 4;

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

                    this->decodedThumb2Instruction._MSRInstruction = MSR(n, sysm);
                    this->thumb2Class = BRANCH_MISC;
                    this->branch_misc_type = MSR_Instruction;
                
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
                        return;
                    }

                    switch (op)
                    {
                        case 0b0100:
                            this->decodedThumb2Instruction._DSBInstruction = DSB(option);
                            this->thumb2Class = BRANCH_MISC;
                            this->branch_misc_type = BL;

                            break;

                        case 0b0101:
                            this->decodedThumb2Instruction._DMBInstruction = DMB(option);
                            this->thumb2Class = BRANCH_MISC;
                            this->branch_misc_type = BL;
                            break;

                        case 0b0110:
                            this->decodedThumb2Instruction._ISBInstruction = ISB(option);
                            break;

                        default:
                            break;
                    }
                }
                // MRS
                else if ((op1 & 0b1111110) == 0b0111110 &&  ((op2 & 0b001) | (op2 & 0b100)) == 0b000)
                {
                    this->decodedThumb2Instruction._MRSInstruction = MRS(second & 0x7, first & 0xFF);
                    this->thumb2Class = BRANCH_MISC;
                    this->branch_misc_type = MRS_Instruction;
                                        // uint8_t d = second & 0x7;
                    // uint8_t sysm = first & 0xFF;         
                }
                else if ((op1 & 0b1111111) == 0b1111111 && op2 == 0b010)
                {
                    // undefined
                }
                else if (((op2 & 0b001) | (op2 & 0b100)) == 0b101)
                {
                    fprintf(stderr,"BRANCH AND LINK\n");
                    this->decodedThumb2Instruction._BranchLinkInstruction = BranchLinkInstruction(instruction);
                    this->thumb2Class = BRANCH_MISC;
                    this->branch_misc_type = BL;
                }
                break;
            }
            case 0b11111:
            {
                break;
            }
        }
    }
    else
    {
        this->decodedInstructionSize = 2;
        this->instructionType = THUMB1;
        InstrClass instructionClass = classify(instruction);
        this->instructionClass = instructionClass;
        // printf("%d ", this->regs[15]);
        // std::cout << " Thumb1Instruction:" << std::bitset<16>((uint16_t)instruction) << std::endl;
        std::cerr << "Thumb1Instruction:" << std::bitset<16>((uint16_t)instruction) << std::endl;
        // std::cerr << "Thumb1Instruction:" << std::hex << ((uint16_t)instruction) << std::endl;
        // std::cerr << "Classified" << std::endl;
        std::cerr << "Class: " << static_cast<int>(instructionClass) << std::endl;

        switch (instructionClass)
        {
            case InstrClass::SHIFT_IMM:
                handleShiftImmediate(instruction);
                break;

            case InstrClass::ADD_SUB:
                this->decodedInstruction._AddSubImmediateInstruction =
                    AddSubImmediateInstruction(instruction);
                break;

            case InstrClass::MOV_CMP_ADD_SUB:
                this->decodedInstruction.movCmpAddSubInstruction =
                    MovCmpAddSubInstruction(instruction);
                break;

            case InstrClass::ALU:
                this->decodedInstruction._ALUInstruction =
                    ALUInstruction(instruction);
                break;

            case InstrClass::HI_REG:
                this->handleSpecialInstructions(instruction);
                break;

            case InstrClass::LDR_LITERAL:
                this->decodedInstruction.lDRLiteralInstruction = 
                    LDRLiteralInstruction(instruction);
                break;

            case InstrClass::LOAD_STORE_REG:
                this->decodedInstruction.loadStoreRegInstruction = 
                    LoadStoreRegInstruction(instruction);
                break;

            case InstrClass::LOAD_STORE_IMM:
                this->decodedInstruction.loadStoreImmInstruction = 
                    LoadStoreImmInstruction(instruction);
                break;

            case InstrClass::LOAD_STORE_HALF:
              this->decodedInstruction.loadStoreHalfInstruction = 
                    LoadStoreHalfInstruction(instruction);
                break;

            case InstrClass::SP_REL:
                this->decodedInstruction.SpRelativeInstruction = 
                    SpRelativeInstruction(instruction);
                break;

            case InstrClass::ADDR:
                this->decodedInstruction.adrInstruction = 
                    AdrInstruction(instruction);
                break;

            case InstrClass::MISC:
                handleMisc(instruction);
                break;

            case InstrClass::MULTIPLE:
                this->decodedInstruction._LoadMultipleInstruction = 
                    LoadMultipleInstruction(instruction);
                break;

            case InstrClass::COND_BRANCH:

                this->decodedInstruction._BranchCondInstruction =
                    BranchCondInstruction(instruction);
                break;

            case InstrClass::UNCOND_BRANCH:
                this->decodedInstruction._BranchUncondInstruction =
                    BranchUncondInstruction(instruction);
                break;

            case InstrClass::UNKNOWN:
            default:
                std::cerr << "UNKNOWN\n";
                break;
        }     
    }
}


void Cpu::print_state(FILE* out) const
{
    fprintf(out, "\n--- CPU STATE ---\n");

    for (uint8_t i = 0; i < 16; i++)
    {
        fprintf(out, "r%-2d: 0x%08X    ", i, this->regs[i]);

        // Every 4 registers, print a newline
        if ((i + 1) % 4 == 0)
        {
            fprintf(out, "\n");
        }
    }

    fprintf(out, "-----------------\n");
    fprintf(out,
            "FLAGS: [ apsr_N:%d | apsr_Z:%d | apsr_C:%d | apsr_V:%d ]\n",
            this->xpsr.N(),
            this->xpsr.Z(),
            this->xpsr.C(),
            this->xpsr.V());

    fprintf(out, "-----------------\n\n");
}

uint32_t Cpu::returnAddress(int exceptionType)
{
    uint32_t result = 0;

    switch (exceptionType)
    {
        case EXCEPTION_NMI:
        {
            result = nextInstrAddr;
            break;
        }

        case EXCEPTION_HARDFAULT:
        {
            if (synchronous_fault)
                result = currentInstrAddr;
            else
                result = nextInstrAddr;

            break;
        }

        case EXCEPTION_SVCALL:
        case EXCEPTION_PENDSV:
        case EXCEPTION_SYSTICK:
        {
            result = nextInstrAddr;
            break;
        }

        default:
        {
            // External interrupts
            if (exceptionType >= 16)
            {
                result = nextInstrAddr;
            }
            else
            {
                assert(false && "Unknown exception number");
            }
            break;
        }
    }

    // Return address must always be halfword aligned
    result &= ~1u;
    fprintf(stderr, "Result %x\n", result);

    return result;
}

void Cpu::pushStack(int ExceptionType)
{
    fprintf(stderr, "pusing stack \n");
    // while(1);
    uint32_t frameptr;
    uint32_t frameptralign;

    // Select active stack pointer
    if (this->control.SPSEL && this->currentMode == Mode::MODE_HANDLER)
    {
        // Save original alignment bit
        frameptralign = (this->psp >> 2) & 1;
        // Allocate 0x20 bytes and align to 8-byte boundary
        this->psp = (this->psp - 0x20) & ~0x7u;

        frameptr = this->psp;
    }
    else
    {
        frameptralign = (this->msp >> 2) & 1;

        this->msp = (this->msp - 0x20) & ~0x7u;

        frameptr = this->msp;
    }

    fprintf(stderr, "frameptr %x\n", frameptr);
//
            // Hardware-defined stack frame layout
            //    fprintf(stderr, "pushing registers done!\n");
    write32(frameptr + 0x00, regs[0]);
    write32(frameptr + 0x04, regs[1]);
    write32(frameptr + 0x08, regs[2]);
    write32(frameptr + 0x0C, regs[3]);
    write32(frameptr + 0x10, regs[12]);
    write32(frameptr + 0x14, regs[14]); // LR
    write32(frameptr + 0x18, returnAddress(ExceptionType));

    // Insert alignment bit into stacked xPSR bit 9
    uint32_t stacked_psr = (xpsr.value & 0xFFFFFDFFu) |(frameptralign << 9);

    write32(frameptr + 0x1C, stacked_psr);

    fprintf(stderr, "pushing registers done! xpsr: %x\n", xpsr.value);
    // while(1);
    fprintf(stderr, "mode: %d\n", this->currentMode);

    // while(1);
    if (this->currentMode == Mode::MODE_HANDLER)
    {
        
        regs[14] = 0xFFFFFFF1;
    }
    else
    {
        if (this->control.SPSEL == 0)
        {
            regs[14] = 0xFFFFFFF9;
        }
        else
        {
            regs[14] = 0xFFFFFFFD;
        }
    }
}

void Cpu::exceptionTaken(int32_t exceptionNumber)
{
    for (int i = 0; i <= 3; i++)
    {
        regs[i] = 0;
    }

    regs[12] = 0;

    this->xpsr.setAPSR(0);

    // Enter Handler mode
    currentMode = Mode::MODE_HANDLER;

    // IPSR<5:0> = ExceptionNumber<5:0>
    this->xpsr.setIPSR(exceptionNumber & 0x3F);

    // Use Main Stack Pointer
    control.SPSEL = 0;

    // CONTROL.nPRIV unchanged

    // Mark exception active
    exceptionActive[exceptionNumber] = true;

    // Update system control state
    // SCS_UpdateStatusRegs();

    // Wake Event Register
    // SetEventRegister();

    // Thumb1Instruction Synchronization Barrier
    // InstructionSynchronizationBarrier(0xF);

    // Load vector table base
    uint32_t vectorTable = 0x0;

    // Read handler address from vector table
    uint32_t handler = read32(vectorTable + (4 * exceptionNumber));
    
    fprintf(stderr, "exceptionNumber: %d\n", exceptionNumber);
    fprintf(stderr, "handler: %x\n", handler);
    // while(1);
    // Branch to handler
    this->BLXWritePC(handler);

}

void Cpu::BLXWritePC(uint32_t address)
{   
    this->xpsr.setT(address & 0b1);
    this->regs[15] = address & ~1u;
    std::cerr << "PC: after blxwrite: " << regs[15] << std::endl;
}

void Cpu::exceptionEntry(int32_t ExceptionType)
{
    fprintf(stderr, "Entering exception\n");
    uint16_t frameptraling = 0;

    if (this->control.SPSEL == 1 && this->currentMode == Mode::MODE_THREAD)
    {
        frameptraling = this->psp;
    }
    // NOTE: PushStack() can abandon memory accesses if a fault occurs during the stacking
    // sequence.
    // Exception entry is modified according to the behavior of a derived exception.

    // while(1);

    pushStack(ExceptionType);
    exceptionTaken(ExceptionType); // ExceptionType is encoded as its exception number
}

void Cpu::deActivate(uint32_t exceptionNumber)
{
    exceptionActive[exceptionNumber] = false;

    // PRIMASK unchanged on exception exit
}

void Cpu::reset()
{
    VTOR = 0;
    for (int i = 0; i <= 12; i++)
    {
        regs[i] = 0xBEEFBEEF; // placeholder unknown value
    }

    uint32_t vectortable = VTOR;
    this->currentMode = Mode::MODE_THREAD;

    // LR = UNKNOWN
    this->regs[14] = 0xFFFFFFFF;

    // APSR = UNKNOWN
    this->xpsr.setAPSR(0);

    // IPSR<5:0> = 0
    this->xpsr.setIPSR(0);
    // PRIMASK.PM = 0
    this->primask = 0;

    // CONTROL.SPSEL = 0
    this->control.SPSEL = false;

    // CONTROL.nPRIV = 0
    this->control.nPRIV = false;

    // Reset system control space registers
    // FIx me
    // ResetSCSRegs();

    // All exceptions inactive
    for (int i = 0; i < 512; i++)
    {
        exceptionPending[i] = false;
        exceptionActive[i] = false;
    }

    // Clear event register
    // Fix me
    // ClearEventRegister();

    // Load initial MSP
    this->msp = read32(vectortable) & 0xFFFFFFFCu;

    // PSP = UNKNOWN aligned
    this->psp = 0;

    // Load reset vector
    uint32_t start = read32(vectortable + 4);

    // Branch to reset handler
    this->regs[15] = start & ~1u;
}

uint32_t Cpu::exceptionActiveBitCount() const
{
    uint32_t count = 0;

    for (bool active : exceptionActive)
    {
        if (active)
        {
            count++;
        }
    }

    return count;
}

void Cpu::exceptionReturn(uint32_t EXC_RETURN)
{
    // EXC_RETURN[31:4] must all be 1
    if ((EXC_RETURN & 0xFFFFFFF0u) != 0xFFFFFFF0u)
    {
        // UNPREDICTABLE();
        return;
    }

    uint32_t returningExceptionNumber = this->xpsr.ipsr() & 0x3F;

    uint32_t nestedActivation = exceptionActiveBitCount();

    // Returning exception must actually be active
    if (!exceptionActive[returningExceptionNumber])
    {
        // UNPREDICTABLE();
        return;
    }

    uint32_t frameptr = 0;

    switch (EXC_RETURN & 0xF)
    {
        //--------------------------------------------------
        // Return to Handler mode
        //--------------------------------------------------
        case 0x1:
        {
            if (nestedActivation == 1)
            {
                // UNPREDICTABLE();
                return;
            }

            frameptr = this->psp;

            this->currentMode = Mode::MODE_HANDLER;

            this->control.SPSEL = 0;

            break;
        }

        //--------------------------------------------------
        // Return to Thread mode using MSP
        //--------------------------------------------------
        case 0x9:
        {
            if (nestedActivation != 1)
            {
                return;
            }

            frameptr = this->psp;

            this->currentMode = Mode::MODE_THREAD;

            this->control.SPSEL = 0;

            break;
        }

        //--------------------------------------------------
        // Return to Thread mode using PSP
        //--------------------------------------------------
        case 0xD:
        {
            if (nestedActivation != 1)
            {
                return;
            }

            frameptr = this->psp;

            this->currentMode = Mode::MODE_THREAD;

            this->control.SPSEL = 1;

            break;
        }

        default:
        {
            return;
        }
    }

    //------------------------------------------------------
    // Exception no longer active
    //------------------------------------------------------
    deActivate(returningExceptionNumber);

    //------------------------------------------------------
    // Restore stacked registers
    //------------------------------------------------------
    PopStack(frameptr, EXC_RETURN);

    //------------------------------------------------------
    // Validate IPSR consistency
    //------------------------------------------------------
    uint32_t ipsrException = this->xpsr.ipsr() & 0x3F;

    if (this->currentMode == Mode::MODE_HANDLER)
    {
        if (ipsrException == 0)
        {
            return;
        }
    }
    else
    {
        if (ipsrException != 0)
        {
            return;
        }
    }

    //------------------------------------------------------
    // Set event register
    //------------------------------------------------------
    // SetEventRegister();

    //------------------------------------------------------
    // Thumb1Instruction synchronization barrier
    //------------------------------------------------------
    // InstructionSynchronizationBarrier();

    //------------------------------------------------------
    // Sleep-on-exit behavior
    //------------------------------------------------------
    // if (this->currentMode == Mode::MODE_THREAD && SCR.SLEEPONEXIT)
    {
        // SleepOnExit();
    }
}

void Cpu::PopStack(uint32_t frameptr, uint32_t EXC_RETURN)
{
    //--------------------------------------------------
    // Restore stacked registers
    //--------------------------------------------------

    this->regs[0]  = read32(frameptr + 0x00);
    regs[1]  = read32(frameptr + 0x04);
    regs[2]  = read32(frameptr + 0x08);
    regs[3]  = read32(frameptr + 0x0C);
    regs[12] = read32(frameptr + 0x10);
    regs[14]    = read32(frameptr + 0x14);
    uint32_t pc  = read32(frameptr + 0x18);
    uint32_t psr = read32(frameptr + 0x1C);

    //--------------------------------------------------
    // Thumb bit validation
    //--------------------------------------------------

    // ARM pseudocode:
    // if pc<0> == '1' then UNPREDICTABLE;

    if ((pc) == 0)
    {
        // UNPREDICTABLE();
        return;
    }

    //--------------------------------------------------
    // Branch to restored PC
    //--------------------------------------------------

    this->regs[15] = pc;

    //--------------------------------------------------
    // Restore stack pointer
    //--------------------------------------------------

    uint32_t align =
        ((psr >> 9) & 1u) << 2;

    switch (EXC_RETURN & 0xF)
    {
        //----------------------------------------------
        // Return to Handler using MSP
        //----------------------------------------------
        case 0x1:
        {
            this->msp =
                (msp + 0x20) | align;
            break;
        }

        //----------------------------------------------
        // Return to Thread using MSP
        //----------------------------------------------
        case 0x9:
        {
            this->msp =
                ( this->msp + 0x20) | align;
            break;
        }

        //----------------------------------------------
        // Return to Thread using PSP
        //----------------------------------------------
        case 0xD:
        {
             this->psp =
                (this->psp + 0x20) | align;
            break;
        }

        default:
        {
            // UNPREDICTABLE();
            return;
        }
    }

    //--------------------------------------------------
    // Restore APSR flags
    //--------------------------------------------------

    xpsr.setN((psr >> 31) & 1u);

    xpsr.setZ((psr >> 30) & 1);

    xpsr.setC((psr >> 29) & 1u);

    xpsr.setV((psr >> 28) & 1u);

    //--------------------------------------------------
    // Determine if forced thread state applies
    //--------------------------------------------------

    bool force_thread =
        (this->currentMode == Mode::MODE_THREAD) &&
        (control.nPRIV == 1);

    //--------------------------------------------------
    // Restore IPSR
    //--------------------------------------------------

    if (force_thread)
    {
        xpsr.setIPSR(0);
    }
    else
    {
        xpsr.setIPSR(psr & 0x3F);
    }

    //--------------------------------------------------
    // Restore EPSR epsr_T-bit
    //--------------------------------------------------

    xpsr.setT((psr >> 24) & 1u);
}

void Cpu::handleAsyncrnousExceptions(void)
{
    fprintf(stderr, "Handling pending asynchronous exceptions\n");
    if (exceptionPending[15])
    {
        fprintf(stderr, "Handling systick\n");
        exceptionPending[15] = false;

        exceptionEntry(15);
        // while(1);
    }
}

bool Cpu::handleSyncrnousExceptions(void)
{
    // highest priority exception selection later
    fprintf(stderr, "Handling pending syncrnous exceptions\n");
    if (exceptionPending[11])
    {
        fprintf(stderr, "Handling svc\n");
        exceptionPending[11] = false;

        // while(1);
        exceptionEntry(11);
    }

    return true;
}

 void Cpu::tick(void)
{
    // disabled
    if (! (scs.systick.SYST_CSR & scs.systick.CSR_ENABLE))
    {
        fprintf(stderr, "SYSTICK NOT enabled!\n");

        return;
    }

    if (scs.systick.SYST_CVR == 0)
    {
        // reload
        scs.systick.SYST_CVR = scs.systick.SYST_RVR;

        // set COUNTFLAG
        scs.systick.SYST_CSR |= scs.systick.CSR_COUNTFLAG;

        // generate SysTick exception
        if (scs.systick.SYST_CSR & scs.systick.CSR_TICKINT)
        {
            exceptionPending[15] = true;
        }
    }
    else
    {
        scs.systick.SYST_CVR--;
        
        fprintf(stderr, "SysTick SYST_CVR: %d\n", scs.systick.SYST_CVR);
    }
}
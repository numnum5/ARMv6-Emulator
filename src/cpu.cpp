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
        fprintf(this->output_file, "%c", value & 0xFF);
        fflush(this->output_file);
        return;
    }

    // Normal access
    address = address - RAM_BASE;

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
    printf("write16(): address: %x\n", address);
    address = address - RAM_BASE;
    this->ram[address] = value & 0xFF;
    this->ram[address + 1] = (value >> 8) & 0xFF;
}

void Cpu::write8(uint32_t address, uint8_t value)
{
        // UART semihosting access
    if(address == UART_DR)
    {
        fprintf(this->output_file, "%c", value);
        fflush(this->output_file);
        return;
    }
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
        uint32_t data = this->flash[address] | (this->flash[address + 1] << 8);
        return data;
    }
    else
    {
        address = address - RAM_BASE;
        uint32_t data = this->ram[address] | (this->ram[address + 1] << 8);
        return data;
    }
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

void Cpu::setAPSRValues(bool c, bool n, bool v, bool z)
{
    this->xpsr.setC(c);
    this->xpsr.setN(n);
    this->xpsr.setV(v);
    this->xpsr.setZ(z);
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
                printf("SRType_ROR\n");
                while(1);

                break;
            case SRType_RRX:
            printf("SRType_RRX\n");
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

bool Cpu::is32bitInstruction(uint32_t instruction)
{
    uint8_t thumb_mode = (instruction >> 11) & 0x1F; 

    if (thumb_mode == 0b11101 || thumb_mode == 0b11110 || thumb_mode == 0b11111)
    {
        return true;
    }

    return false;
}


void Cpu::setPrimaskPM(bool value)
{
    this->primask |= value;
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
            // result = nextInstrAddr;
            // break;
        }

        case EXCEPTION_HARDFAULT:
        {
            // if (synchronous_fault)
            //     // result = currentInstrAddr;
            // else
            // {

            // }
            //     // result = nextInstrAddr;

            break;
        }

        case EXCEPTION_SVCALL:
        case EXCEPTION_PENDSV:
        case EXCEPTION_SYSTICK:
        {
            // result = nextInstrAddr;
            break;
        }

        default:
        {
            // External interrupts
            if (exceptionType >= 16)
            {
                // result = nextInstrAddr;
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

// void tick()
// {
//     // disabled
//     if (! (scs.systick.SYST_CSR & scs.systick.CSR_ENABLE))
//     {
//         fprintf(stderr, "SYSTICK NOT enabled!\n");

//         return;
//     }

//     if (scs.systick.SYST_CVR == 0)
//     {
//         // reload
//         scs.systick.SYST_CVR = scs.systick.SYST_RVR;

//         // set COUNTFLAG
//         scs.systick.SYST_CSR |= scs.systick.CSR_COUNTFLAG;

//         // generate SysTick exception
//         if (scs.systick.SYST_CSR & scs.systick.CSR_TICKINT)
//         {
//             exceptionPending[15] = true;
//         }
//     }
//     else
//     {
//         scs.systick.SYST_CVR--;
        
//         fprintf(stderr, "SysTick SYST_CVR: %d\n", scs.systick.SYST_CVR);
//     }
// }
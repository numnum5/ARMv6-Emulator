#include <iostream>
#include <vector>
#include <limits>
#include <bitset>
#include <bit>
#include "system.hpp"
#include "registers.hpp"

using bit = bool;
enum class Opcode : uint16_t
{
    INVALID,

    //
    // Branch / control flow
    //
    B,
    B_COND,
    BL,
    BLX,
    BX,

    //
    // Arithmetic / data processing
    //
    ADC,

    ADD_REG,
    ADD_IMM_T1,
    ADD_IMM_T2,
    ADD_SP_T1,

    ADR,

    AND,

    ASR_IMM,
    ASR_REG,

    BIC,

    CMN,

    CMP_REG,
    CMP_IMM,

    EOR,

    LSL_IMM,
    LSL_REG,

    LSR_IMM,
    LSR_REG,

    MOV_IMM,
    MOV_REG,

    MUL,
    MVN,
    NEG,
    ORR,
    ROR,
    RSB,
    SBC,

    SUB_REG,
    SUB_IMM_T1,
    SUB_IMM_T2,

    TST,

    //
    // Load / store
    //
    LDR_LITERAL,

    LDR_IMM,
    LDR_IMM_T2,
    LDR_REG,

    LDRB,
    LDRB_IMM,

    LDRH,
    LDRH_IMM,

    LDRSB,
    LDRSH,

    STR_IMM,
    STR_IMM_T2,
    STR_REG,

    STRB_REG,
    STRB_IMM,

    STRH_REG,
    STRH_IMM,

    //
    // Multiple register transfer
    //
    LDMIA,
    STMIA,
    PUSH,
    POP,

    //
    // System register access
    //
    MRS,
    MSR,

    //
    // Exception / system
    //
    BKPT,
    SVC,

    //
    // Barriers
    //
    DMB,
    DSB,
    ISB,

    //
    // Misc / hints
    //
    CPS,
    NOP,
    SEV,
    WFE,
    WFI,
    YIELD,

    //
    // Extend / byte manipulation
    //
    REV,
    REV16,
    REVSH,

    SXTB,
    SXTH,
    UXTB,
    UXTH,
    SUB_SP_T1,
    ADD_SP_T2
};

enum class InstrClass {
    SHIFT_IMM,        // LSL, LSR, ASR
    ADD_SUB,          // add/sub (reg + imm3)
    MOV_CMP_ADD_SUB,  // imm8 ops
    ALU,              // AND, ORR, EOR, etc.
    HI_REG,           // high register ops + BX
    LDR_LITERAL,      
    LOAD_STORE_REG,
    LOAD_STORE_IMM,
    LOAD_STORE_HALF,
    SP_REL,
    ADDR,
    MISC,
    MULTIPLE,
    COND_BRANCH,
    SVC,
    UNCOND_BRANCH,
    UNKNOWN
};
typedef enum 
{
    SRType_LSL,
    SRType_LSR,
    SRType_ASR,
    SRType_ROR,
    SRType_RRX

} SRType;


struct AddCarryResult
{
    uint32_t result;
    bool carry_out;
    bool overflow;
};

typedef struct 
{
    SRType type;
    uint8_t n;
} DecodeImmShiftResult;

typedef struct
{
    uint32_t result;
    bool carry_out;
} Shift_c;

enum class Mode
{
    MODE_HANDLER,
    MODE_THREAD
};

struct Control
{
    uint32_t nPRIV : 1;
    uint32_t SPSEL : 1;
    uint32_t reserved : 30;
};

struct XPSR
{
    uint32_t value;

    //----------------------------------
    // IPSR
    //----------------------------------

    uint32_t ipsr() const
    {
        return value & 0x3F;
    }

    uint32_t apsr() const
    {
        return (value >> 28) & 0xF;
    }

    void setNZCVQ(uint32_t v)
    {
        // Bits 31:27
        value &= ~(0x1Fu << 27);
        value |= (v & 0x1Fu) << 27;
    }

    void setIPSR(uint32_t n)
    {
        value &= ~0x3Fu;
        value |= (n & 0x3F);
    }

    void setAPSR(uint32_t apsr)
    {
        value &= ~(0xFu << 28);

        value |= (apsr & 0xF) << 28;
    }

    //----------------------------------
    // T bit
    //----------------------------------

    bool T() const
    {
        return (value >> 24) & 1u;
    }

    void setT(bool b)
    {
        value &= ~(1u << 24);
        value |= (uint32_t)b << 24;
    }

    //----------------------------------
    // APSR flags
    //----------------------------------

    bool N() const { return (value >> 31) & 1u; }
    bool Z() const { return (value >> 30) & 1u; }
    bool C() const { return (value >> 29) & 1u; }
    bool V() const { return (value >> 28) & 1u; }

    void setN(bool b)
    {
        value &= ~(1u << 31);
        value |= (uint32_t)b << 31;
    }

    void setZ(bool b)
    {
        value &= ~(1u << 30);
        value |= (uint32_t)b << 30;
    }

    void setC(bool b)
    {
        value &= ~(1u << 29);
        value |= (uint32_t)b << 29;
    }

    void setV(bool b)
    {
        value &= ~(1u << 28);
        value |= (uint32_t)b << 28;
    }
};



enum ExceptionType
{
    EXCEPTION_RESET     = 1,
    EXCEPTION_NMI       = 2,
    EXCEPTION_HARDFAULT = 3,
    EXCEPTION_SVCALL    = 11,
    EXCEPTION_PENDSV    = 14,
    EXCEPTION_SYSTICK   = 15
};


// Pipeline Registers (Latches between stages)
typedef struct {
    uint32_t instruction;
    uint32_t pc;
    bool stall;
    bool valid;
} FetchDecodeLatch;

#include <unordered_map>

enum class Execute : uint8_t
{
    ALU,
    MEMORY
};


enum MultipleInstrucitonState
{
    TRANSFER,
    SETUP,
    LINK
};


typedef struct {
    Opcode opcode;
    uint32_t pc;
    uint32_t branch_pc;
    uint8_t destination;
    uint32_t rd;
    uint32_t imm32;
    uint8_t n;
    uint8_t m;
    uint8_t t;
    std::vector<uint8_t> registers_push_pop;
    uint16_t register_list_count;
    uint16_t register_list; 
    std::vector<uint8_t> register_list_decoded; 
    std::unordered_map<uint8_t, uint32_t> registers_read;
    Execute state;
    SRType shift_t;
    uint8_t shift_n;
    uint32_t write_address;
    uint32_t read_address;
    uint16_t pop_push_cycles;
    uint8_t pop_push_iteration;
    uint32_t pop_push_address;
    uint8_t cond;
    MultipleInstrucitonState popState;

    bool push_pop_M;
    bool valid;
    bool reg_write;
    bool reg_read;
    bool mem_read;
    bool mem_write;
    bool wback;

} DecodeExecuteLatch;

struct Pipeline
{
    FetchDecodeLatch   FD_latch;
    DecodeExecuteLatch DE_latch;
};

class Cpu
{  
    public:
        static constexpr uint32_t FLASH_BASE = 0x00000000;
        static constexpr uint32_t FLASH_SIZE = 64 * 1024;                    
        //  static constexpr uint32_t RAM_BASE   = 0x00000000;
        static constexpr uint32_t RAM_BASE   = 0x20000000;
        static constexpr uint32_t RAM_SIZE   = 16 * 1024;

        uint8_t exception_request;
        uint8_t exception_number;
        bool synchronous_fault;
        bool exceptionActive[512];
        bool exceptionPending[512];
        std::vector<uint8_t> flash;
        std::vector<uint8_t> ram;
        Registers registers;
        uint32_t regs[16];
        FILE * output_file;
        
        XPSR xpsr;
        SCS scs;

        uint32_t msp;
        uint32_t psp;
        uint32_t primask;
        Control control;
        Mode currentMode;
        InstrClass instructionClass;
        DecodeImmShiftResult decodeImmShiftResult;
        uint32_t VTOR;
        uint32_t fetch_pc;
        Pipeline pipeline;
        bool stall = false;
        bool flush = false;
        uint32_t branch_pc;
        uint32_t cycle;
        bool branch_taken = false;

        void writeFlash16(uint32_t address, uint16_t value);
        uint32_t BXWritePC2(uint32_t address);
        void execute_final(DecodeExecuteLatch & DE_latch, DecodeExecuteLatch & next);
        void test(uint32_t cycles);
        void step();
        Opcode decode_special(uint8_t opcode, uint8_t d, uint8_t m, bool H1);
        Opcode decode_alu(uint8_t opcode);
        Opcode decode_load_store_reg(uint8_t opcode);
        Opcode decode_load_store_half(bool opcode);
        Opcode decode_load_store_imm(uint8_t opcode);
        bool stage_execute(Pipeline & next);
        void stage_decode(Pipeline & next);
        void stage_fetch(uint8_t* flash, Pipeline& next);
        uint32_t pc_adder(Pipeline & next);
        void commit(Pipeline & next);
        void handle_32bit_instruction(uint32_t instruction, DecodeExecuteLatch & decode);
        void decode_final(Pipeline & next, uint32_t instruction, std::unordered_map<uint8_t, uint32_t> & registers_read);
        Opcode decode_misc(uint16_t instr, DecodeExecuteLatch & DE_latch, std::unordered_map<uint8_t, uint32_t> & registers_read);
        void writeFlash32(uint32_t address, uint16_t value);
        Cpu(size_t ram_size, size_t flash_size);
        void setPrimaskPM(bool value);
        bool conditionPassed(uint8_t cond) const;
        void write32(uint32_t address, uint32_t value);
        void write16(uint32_t address, uint16_t value);
        void write8(uint32_t address, uint8_t value);

        uint8_t read8(uint32_t address) const;
        uint16_t read16(uint32_t address) const;
        uint32_t read32(uint32_t address) const;
        uint32_t read32Flash(uint32_t address) const;
        uint32_t getSP(void) const;
        uint32_t exceptionActiveBitCount(void) const;
        bool currentModeIsPrivileged(void);
        void setAPSRValues(bool c, bool n, bool v, bool z);

        uint8_t currentCond(uint32_t instruction);
        uint32_t shift(uint32_t value, SRType type, uint8_t amount, bit carry_in);
        uint32_t sign_extend(uint32_t value, int bits);
        DecodeImmShiftResult decodeImmShift(uint8_t type, uint8_t imm5);
        Shift_c shift_c(uint32_t value, SRType type, uint8_t amount, bit carry_in);
        AddCarryResult addWithCarry(uint32_t x, uint32_t y, bool carry_in);        
        
        InstrClass classify(uint16_t instr);
        
        bool is32bitInstruction(uint32_t instruction);

        std::pair<uint32_t, bool> LSL_C(uint32_t x, int shift);
        std::pair<uint32_t, bool> ROR_C(uint32_t x, int shift);
        std::pair<uint32_t, bool> LSR_C(uint32_t x, int shift);
        std::pair<uint32_t, bool> ASR_C(uint32_t x, int shift);
        std::pair<uint32_t, bool> RRX_C(uint32_t x, bool carry_in);
    
        uint32_t RRX(uint32_t x, bool carry_in);
        uint32_t ASR(uint32_t x, int shift);
        uint32_t LSR(uint32_t x, int shift);
        uint32_t ROR(uint32_t x, int shift);
        uint32_t LSL(uint32_t x, int shift);
        uint32_t returnAddress(int exceptionType);

        void ALUWritePC(uint32_t address);
        void pushStack(int ExceptionType);
        void exceptionTaken(int32_t exceptionNumber);
        void BLXWritePC(uint32_t address);
        void exceptionEntry(int32_t ExceptionType);
        void BXWritePC(uint32_t address);
        bool handleSyncrnousExceptions(void);
        void handleAsyncrnousExceptions(void);
        void print_state(FILE* out = stderr) const;
        void deActivate(uint32_t exceptionNumber);
        void print_mem();
        void reset(void);
        void exceptionReturn(uint32_t EXC_RETURN);
        void PopStack(uint32_t frameptr, uint32_t EXC_RETURN);

    private:

};

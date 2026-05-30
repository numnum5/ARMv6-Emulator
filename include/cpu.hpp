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
    SEV,
    NOP,
    SVC,
    CPS,
    SUBADD,
    PUSHPOP,
    REV,
    EXTEND

} Misc_type;

typedef enum 
{
    MSR_Instruction,
    MRS_Instruction,
    DSB_Instruction,
    DMB_Instruction,
    ISB_Instruction,
    UDF,
    BL

} Branch_misc_type;

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

struct ShiftImmediateInstruction
{
    uint16_t rd : 3;
    uint16_t rm : 3;
    uint16_t reserved : 5;
    uint16_t identifier : 5;
    ShiftImmediateInstruction() = default;
    ShiftImmediateInstruction(uint16_t instr)
    {
        rd = instr & 0x7;
        rm = (instr >> 3) & 0x7;
        reserved = (instr >> 6) & 0x1F;
        identifier = (instr >> 11) & 0x1F;
    }
};

enum class SpecialOp : uint8_t
{
    ADD,
    CMP,
    MOV,
    BX_BLX
};
struct AddHighInstruction
{
    uint8_t d;
    uint8_t m;

    AddHighInstruction() = default;

    AddHighInstruction(uint8_t d, uint8_t m)
        : d(d), m(m)
    {}
};

struct CmpHighInstruction
{
    uint8_t rn;
    uint8_t rm;

    CmpHighInstruction() = default;

    CmpHighInstruction(uint8_t rn, uint8_t rm)
        : rn(rn), rm(rm)
    {}
};

struct MovHighInstruction
{
    uint8_t d;
    uint8_t m;

    MovHighInstruction() = default;

    MovHighInstruction(uint8_t d, uint8_t m)
        : d(d), m(m)
    {}
};

struct BranchExchangeInstruction
{
    uint8_t rm;
    bool blx;

    BranchExchangeInstruction() = default;

    BranchExchangeInstruction(uint8_t rm, bool blx)
        : rm(rm), blx(blx)
    {}
};

union SpecialInstruction
{
    AddHighInstruction add;
    CmpHighInstruction cmp;
    MovHighInstruction mov;
    BranchExchangeInstruction bx;

    // SpecialInstruction() {}
};

struct MovCmpAddSubInstruction
{
    uint16_t imm8 : 8;
    uint16_t d : 3;
    uint16_t op : 2;
    uint16_t identifier: 3;
    MovCmpAddSubInstruction() = default;
    MovCmpAddSubInstruction(uint16_t instr)
    {
        imm8 = instr & 0xFF;
        d = (instr >> 8) & 0x7;
        op = (instr >> 11) & 0x3;
        identifier = (instr >> 13) & 0x7;
    }
};

struct LoadStoreRegInstruction 
{
    uint16_t rt : 3;
    uint16_t rn : 3;
    uint16_t rm : 3;
    uint16_t opcode : 3;
    uint16_t identifier : 4;
  LoadStoreRegInstruction() = default;
    LoadStoreRegInstruction(uint16_t instr)
    {
        rt = instr & 0x7;
        rn = (instr >> 3) & 0x7;
        rm = (instr >> 6) & 0x7;
        opcode = (instr >> 9) & 0x7;
        identifier = (instr >> 12) & 0xF;
    }
};

struct LDRLiteralInstruction 
{
    uint16_t imm8 : 8;
    uint16_t rt : 3;
    uint16_t identifier : 5;
  LDRLiteralInstruction() = default;
    LDRLiteralInstruction(uint16_t instr)
    {
        imm8 = instr & 0xFF;
        rt = (instr >> 8) & 0x7;
        identifier = (instr >> 11) & 0x1F;
    }
};

struct AddSubImmediateInstruction
{
    uint16_t rd : 3;
    uint16_t rn : 3;
    uint16_t imm3 : 3;
    uint16_t bits_9 : 1;
    uint16_t bits_10 : 1;
    uint16_t identifier : 5;

    AddSubImmediateInstruction() = default;
    AddSubImmediateInstruction(uint16_t instruction)
    {
        // 0b0001110111111011
        rd         = (instruction) & 0x7;
        rn         = (instruction >> 3)  & 0x7;
        imm3       = (instruction >> 6)  & 0x7;
        bits_9     = (instruction >> 9)  & 0x1;
        bits_10    = (instruction >> 10) & 0x1;
        identifier = (instruction >> 11) & 0x1F;
    }
};

struct ALUInstruction
{
    uint16_t rdn : 3;
    uint16_t rm : 3;
    uint16_t opcode : 4;
    uint16_t identifier : 6;
    ALUInstruction() = default;
    ALUInstruction(uint16_t instr)
    {
        rdn = instr & 0x7;
        rm = (instr >> 3) & 0x7;
        opcode = (instr >> 6) & 0xF;
        identifier = (instr >> 10) & 0x3F;
    }
};

struct SpRelativeInstruction
{
    uint16_t imm8 : 8;
    uint16_t rt : 3;
    uint16_t opcode : 1;
    uint16_t identifier : 4;
    SpRelativeInstruction() = default;
    SpRelativeInstruction(uint16_t instr)
    {
        imm8 = instr & 0xFF;
        rt = (instr >> 8) & 0x7;
        opcode = (instr >> 11) & 0x1;
        identifier = (instr >> 12) & 0xF;
    }
};

struct AdrInstruction
{
    uint16_t imm8 : 8;
    uint16_t rd : 3;
    uint16_t bits_11 : 1;
    uint16_t identifier : 4;
    AdrInstruction() = default;
    AdrInstruction(uint16_t instr)
    {
        imm8 = instr & 0xFF;
        rd = (instr >> 8) & 0x7;
        bits_11 = (instr >> 11) & 0x1;
        identifier = (instr >> 12) & 0xF;
    }
};

struct LoadStoreImmInstruction
{
    uint16_t rt : 3;
    uint16_t rn : 3;
    uint16_t imm5 : 5;
    uint16_t opcode : 2;
    uint16_t identifier : 3;
    LoadStoreImmInstruction() = default;
    LoadStoreImmInstruction(uint16_t instr)
    {
        rt = instr & 0x7;
        rn = (instr >> 3) & 0x7;
        imm5 = (instr >> 6) & 0x1F;
        opcode = (instr >> 11) & 0x3;
        identifier = (instr >> 13) & 0x7;
    }
};

struct LoadStoreHalfInstruction
{
    uint16_t rt : 3;
    uint16_t rn : 3;
    uint16_t imm5 : 5;
    uint16_t opcode : 1;
    uint16_t identifier : 4;
    LoadStoreHalfInstruction() = default;
    LoadStoreHalfInstruction(uint16_t instr)
    {
        rt = instr & 0x7;
        rn = (instr >> 3) & 0x7;
        imm5 = (instr >> 6) & 0x1F;
        opcode = (instr >> 11) & 0x1;
        identifier = (instr >> 12) & 0xF;
    }
};

struct CPSInstruction
{
    uint16_t bits_0_3 : 4;
    uint16_t im : 1;
    uint16_t opcode : 4;
    uint16_t identifier : 4;
    CPSInstruction() = default;
    CPSInstruction(uint16_t instr)
    {
        bits_0_3 = instr & 0xF;
        im = (instr >> 4) & 0x1;
        opcode = (instr >> 5) & 0xF;
        identifier = (instr >> 9) & 0xF;
    }
};

struct AddSubSpInstruction
{
    uint16_t imm7 : 7;
    uint16_t bits_7 : 1;
    uint16_t opcode : 4;
    uint16_t identifier : 4;
    AddSubSpInstruction() = default;
    AddSubSpInstruction(uint16_t instr)
    {
        imm7 = instr & 0x7F;
        bits_7 = (instr >> 7) & 0x1;
        opcode = (instr >> 8) & 0xF;
        identifier = (instr >> 12) & 0xF;
    }
};

struct PushPopInstruction
{
    uint16_t register_list : 8;
    uint16_t M : 1;
    uint16_t bits_10_9 : 2;
    uint16_t op : 1;
    uint16_t identifier : 4;
    PushPopInstruction() = default;
    PushPopInstruction(uint16_t instr)
    {
        register_list = instr & 0xFF;
        M = (instr >> 8) & 0x1;
        bits_10_9 = (instr >> 9) & 0x3;
        op = (instr >> 11) & 0x1;
        identifier = (instr >> 12) & 0xF;
    }
};

struct LoadMultipleInstruction
{
    uint16_t register_list : 8;
    uint16_t rn : 3;
    uint16_t op : 1;
    uint16_t identifier : 4;
    LoadMultipleInstruction() = default;
    LoadMultipleInstruction(uint16_t instr)
    {
        register_list = instr & 0xFF;
        rn = (instr >> 8) & 0x7;
        op = (instr >> 11) & 0x1;
        identifier = (instr >> 12) & 0xF;
    }
};

struct ExtendInstruction 
{
    uint16_t rd : 3;
    uint16_t rm : 3;
    uint16_t op : 2;
    uint16_t bits_11_8 : 4;
    uint16_t identifier : 4;
    ExtendInstruction() = default;
    ExtendInstruction(uint16_t instr)
    {
        rd = instr & 0x7;
        rm = (instr >> 3) & 0x7;
        op = (instr >> 6) & 0x3;
        bits_11_8 = (instr >> 8) & 0xF;
        identifier = (instr >> 12) & 0xF;
    }
};

struct RevInstruction
{
    uint16_t rd : 3;
    uint16_t rm : 3;
    uint16_t op : 2;
    uint16_t bits_11_8 : 4;
    uint16_t identifier : 4;
    RevInstruction() = default;
    RevInstruction(uint16_t instr)
    {
        rd = instr & 0x7;
        rm = (instr >> 3) & 0x7;
        op = (instr >> 6) & 0x3;
        bits_11_8 = (instr >> 8) & 0xF;
        identifier = (instr >> 12) & 0xF;
    }
};

struct BranchCondInstruction 
{
    uint16_t imm8 : 8;
    uint16_t cond : 4;
    uint16_t identifier : 4;

    BranchCondInstruction() = default;
    
    BranchCondInstruction(uint16_t instr)
    {
        imm8 = instr & 0xFF;
        cond = (instr >> 8) & 0xF;
        identifier = (instr >> 12) & 0xF;
    }
};

struct BranchUncondInstruction 
{
    uint16_t imm11 : 11;
    uint16_t identifier : 5;

    BranchUncondInstruction() = default;
    
    BranchUncondInstruction(uint16_t instr)
    {
        imm11 = instr & 0x7FF;
        identifier = (instr >> 11) & 0x1F;
    }
};

enum InstructionClassThumb2
{
    BRANCH_MISC
};

union Thumb1Instruction
{
    struct ShiftImmediateInstruction shiftImmediateInstruction;
    SpecialInstruction _SpecialInstruction;
    struct MovCmpAddSubInstruction movCmpAddSubInstruction;
    struct LoadStoreRegInstruction loadStoreRegInstruction;
    struct LDRLiteralInstruction lDRLiteralInstruction;
    struct ALUInstruction _ALUInstruction;
    struct SpRelativeInstruction SpRelativeInstruction;
    struct AdrInstruction adrInstruction;
    struct LoadStoreImmInstruction loadStoreImmInstruction;
    struct LoadStoreHalfInstruction loadStoreHalfInstruction;
    struct CPSInstruction cPSInstruction;


    struct AddSubSpInstruction _AddSubSpInstruction;
    struct PushPopInstruction _PushPopInstruction;
    struct LoadMultipleInstruction _LoadMultipleInstruction;
    struct ExtendInstruction _ExtendInstruction;
    struct RevInstruction _RevInstruction;

    struct BranchCondInstruction _BranchCondInstruction;
    struct BranchUncondInstruction _BranchUncondInstruction;
    struct AddSubImmediateInstruction _AddSubImmediateInstruction;
};

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

struct BranchLinkInstruction
{
    // first 16-bit halfword
    uint32_t imm11 : 11;   // bits [10:0]
    uint32_t J2    : 1;    // bit 11
    uint32_t one2  : 1;    // bit 12 (should be 1)
    uint32_t J1    : 1;    // bit 13
    uint32_t op2  : 2;    // bits [15:14] (should be 11)

    // second 16-bit halfword
    uint32_t imm10 : 10;   // bits [25:16]
    uint32_t S     : 1;    // bit 26
    uint32_t op1    : 5;    // bits [31:27] (should be 11110)

    BranchLinkInstruction() = default;
    
    BranchLinkInstruction(uint32_t instruction)
    {
        uint16_t first  = instruction & 0xFFFF;
        uint16_t second = instruction >> 16;

        this->op1 = (instruction >> 20) & 0x7F;
        this->op2 = (instruction >> 12) & 0b111;
        this->S  = (first >> 10) & 1;
        this->imm10 = first & 0x03FF;
        this->J1 = (second >> 13) & 1;
        this->J2 = (second >> 11) & 1;
        this->imm11 = second & 0x07FF;
    }
};

struct DSB
{
    // first 16-bit halfword
    uint32_t option : 4;   // bits [10:0]
    uint32_t reservered : 28;

    DSB() = default;
    
    DSB(uint8_t option)
    {
        this->option = option;
        reservered = 0;
    }
};

struct DMB
{
    // first 16-bit halfword
    uint32_t option : 4;   // bits [10:0]
    uint32_t reservered : 28;

    DMB() = default;
    
    DMB(uint8_t option)
    {
        this->option = option;
        reservered = 0;
    }
};

struct ISB
{
    // first 16-bit halfword
    uint32_t option : 4;   // bits [10:0]
    uint32_t reservered : 28;

    ISB() = default;
    
    ISB(uint8_t option)
    {
        this->option = option;
        reservered = 0;
    }
};

struct MSR
{
    uint32_t sysm : 8;
    uint32_t reserved0 : 8;
    uint32_t rn : 4;
    uint32_t reserved : 12;

    MSR() = default;
    
    MSR(uint8_t rn, uint8_t sysm)
    {
        this->rn = rn;
        this->sysm = sysm;
        this->reserved0 = 0;
        this->reserved = 0;
    }
};


struct MRS
{
    uint32_t sysm : 8;
    uint32_t rd : 4;
    uint32_t reserved : 22;

    MRS() = default;
    
    MRS(uint8_t rd, uint8_t sysm)
    {
        this->rd = rd;
        this->sysm = sysm;

    }
};

union Thumb2Instruction
{
    BranchLinkInstruction _BranchLinkInstruction;
    DSB _DSBInstruction;
    DMB _DMBInstruction;
    ISB _ISBInstruction;
    MRS _MRSInstruction;
    MSR _MSRInstruction;
};

enum InstructionType
{
    THUMB1,
    THUMB2
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
struct FetchStage
{
    bool valid = false;
    uint32_t pc_out = 0;
    uint32_t instruction_out = 0;
    uint32_t size = 0;
    void update(uint32_t pc_in, uint32_t instruction_in)
    {
        this->pc_out = pc_in;
        this->instruction_out = instruction_in;
    }
};


struct Decoded
{
    InstructionType instructionType; // thumb1 or thumb2
    Thumb2Instruction decodedThumb2Instruction;
    Thumb1Instruction decodedInstruction;
    InstructionClassThumb2 thumb2Class;
    InstrClass instructionClass;
    DecodeImmShiftResult decodeImmShiftResult;
    Misc_type misc_type;
    Branch_misc_type branch_misc_type;
    SpecialOp specialOpType;

    uint8_t n;
    uint8_t m;
    uint8_t d;
    uint8_t t;

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
    // Decoded decoded;
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

        uint32_t nextInstrAddr;
        uint32_t currentInstrAddr;
        uint8_t exception_request;
        uint8_t exception_number;
        uint8_t decodedInstructionSize;

        bool synchronous_fault;
        bool exceptionActive[512];
        bool exceptionPending[512];

        std::vector<uint8_t> flash;
        std::vector<uint8_t> ram;
        Registers registers;
        uint32_t regs[16];
        
        XPSR xpsr;
        SCS scs;

        uint32_t msp;
        uint32_t psp;
        uint32_t primask;

        Control control;
        Mode currentMode;

        Thumb2Instruction decodedThumb2Instruction;
        InstructionClassThumb2 thumb2Class;
    
        Thumb1Instruction decodedInstruction;
        InstrClass instructionClass;
        
        DecodeImmShiftResult decodeImmShiftResult;
        
        Misc_type misc_type;

        Branch_misc_type branch_misc_type;
        
        SpecialOp specialOpType;

        InstructionType instructionType;

        uint32_t fetched_instruction;
        uint32_t VTOR;


    uint32_t fetch_pc;





    Pipeline pipeline;
    FetchDecodeLatch   FD_latch = {};
    DecodeExecuteLatch DE_latch = {};
            bool execute22 = false;
        bool stall = false;
        bool flush = false;

        uint32_t branch_pc;
        uint32_t cycle;
        bool branch_taken = false;
    void writeFlash16(uint32_t address, uint16_t value);
uint32_t BXWritePC2(uint32_t address);
void execute_final(DecodeExecuteLatch & DE_latch, DecodeExecuteLatch & next);
void executeMovCmpAddSub2(const MovCmpAddSubInstruction & decoded);
 void test(uint32_t cycles);
 Opcode decode_special(uint8_t opcode, uint8_t d, uint8_t m, bool H1);
 
Opcode decode_alu(uint8_t opcode);
 Opcode decode_load_store_reg(uint8_t opcode);
 Opcode decode_load_store_half(bool opcode);
Opcode decode_load_store_imm(uint8_t opcode);
 bool stage_execute(Pipeline& next);
 void stage_decode(Pipeline& next);
 void stage_fetch(uint8_t* flash, Pipeline& next);
 uint32_t pc_adder(Pipeline & next);
  void commit(Pipeline & next);
   void handle_32bit_instruction(uint32_t instruction, DecodeExecuteLatch & decode);
   void decode_final(Pipeline & next, uint32_t instruction, std::unordered_map<uint8_t, uint32_t> & registers_read);
Opcode decode_misc(uint16_t instr, DecodeExecuteLatch & DE_latch, std::unordered_map<uint8_t, uint32_t> & registers_read);


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
        
        void tick(void);
        void setAPSRValues(bool c, bool n, bool v, bool z);
        void handleSpecialInstructions(uint16_t instruction);

        uint8_t currentCond(uint32_t instruction);
        uint32_t shift(uint32_t value, SRType type, uint8_t amount, bit carry_in);
        uint32_t sign_extend(uint32_t value, int bits);
        DecodeImmShiftResult decodeImmShift(uint8_t type, uint8_t imm5);
        Shift_c shift_c(uint32_t value, SRType type, uint8_t amount, bit carry_in);
        AddCarryResult addWithCarry(uint32_t x, uint32_t y, bool carry_in);        
        InstrClass classify(uint16_t instr);

        bool inITBlock(void);
        bool is32bitInstruction(uint32_t instruction);

        void handleMisc2(uint16_t instr, Decoded & decoded);
        void handleSpecialInstructions2(uint16_t instruction, Decoded & decoded);
        void handleShiftImmediate2(uint16_t instr, Decoded & decoded);
        void handleShiftImmediate(uint16_t instr);
        void handleMisc(uint16_t instr);

        void executeMisc(void);
        void executeAdr(void);
        void executeLoadStoreReg(void);
        void executeSpRelative(void);
        void executeALUinstr(void);
        void executeLoadStoreHalf(void);
        void executeLoadStoreImm(void);
        void executeLDRLiteral(void);
        void executeMultiple(void);
        void executeMovCmpAddSub(void);
        void executeUncondBranch(void);
        void executeCondBranch(void);
        void executeAddSub(void);
        void executeShiftImmediate(void);
        void executeSpecialInstructions(void);    

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

        // The 3 stages of the CPU processing
        uint32_t fetch(void);
        uint32_t fetch2(uint32_t pc);
        Decoded decode2(uint32_t instruction);
        bool execute2(Decoded decoded, uint32_t pc);
        void decode(void);
        void execute(void);
        void print_state(FILE* out = stderr) const;
        void deActivate(uint32_t exceptionNumber);
void print_mem();
        void reset(void);
        void exceptionReturn(uint32_t EXC_RETURN);
        void PopStack(uint32_t frameptr, uint32_t EXC_RETURN);

    private:

};

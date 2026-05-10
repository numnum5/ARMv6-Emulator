#include <iostream>
#include <vector>
#include <limits>
#include <bitset>
#include <bit>
#include "registers.hpp"

using bit = bool;

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

constexpr uint8_t PC_INDEX = 15;


typedef struct 
{
    bit t;
} EPSR;

typedef struct
{
    bit N;
    bit C;
    bit Z;
    bit V;
    bit Q;
} ASPR;

struct AddCarryResult
{
    uint32_t result;
    bool carry_out;
    bool overflow;
};

typedef struct {
    SRType type;
    uint8_t n;
} DecodeImmShiftResult;

typedef struct
{
    uint32_t result;
    bool carry_out;
} Shift_c;


class Cpu
{
    public:
        static constexpr uint32_t FLASH_BASE = 0x00000000;
        static constexpr uint32_t FLASH_SIZE = 64 * 1024;

        static constexpr uint32_t RAM_BASE   = 0x20000000;
        static constexpr uint32_t RAM_SIZE   = 16 * 1024;
        // uint8_t flash[0x1000];

        std::vector<uint8_t> flash;
        std::vector<uint8_t> ram;
        Registers registers;
        uint32_t regs[16];
        // uint8_t ram[0x1000];
        ASPR aspr;
        EPSR espr;
        uint32_t xpsr;
        uint32_t msp;
        uint32_t psp;

        Cpu(size_t ram_size, size_t flash_size);

        uint32_t getSP(void) const;
        bool conditionPassed(uint8_t cond) const;
        void write32(uint32_t address, uint32_t value);
        void write16(uint32_t address, uint16_t value);
        void write8(uint32_t address, uint8_t value);
        uint8_t read8(uint32_t address) const;
        uint16_t read16(uint32_t address) const;
        uint32_t read32(uint32_t address) const;
        uint32_t read32v2(uint32_t address) const;
        uint32_t read32Flash(uint32_t address) const;
        InstrClass classify(uint16_t instr);

        
        void handleSpecialInstructions(uint16_t instruction);
        uint32_t fetch(void) const;
        uint8_t currentCond(uint32_t instruction);
        uint32_t shift(uint32_t value, SRType type, uint8_t amount, bit carry_in);
        uint32_t sign_extend(uint32_t value, int bits);
        DecodeImmShiftResult decodeImmShift(uint8_t type, uint8_t imm5);
        Shift_c shift_c(uint32_t value, SRType type, uint8_t amount, bit carry_in);
        AddCarryResult addWithCarry(uint32_t x, uint32_t y, bool carry_in);

        bool inITBlock(void);
        bool is32bitInstruction(uint8_t thumb_mode);
        void HandleALUinstr(uint16_t instruction);
        void handleLoadStoreHalf(uint16_t instr);
        void handleLoadStoreImm(uint16_t instr);
        void handleLDRLiteral(uint16_t instruction);
        void handleMultiple(uint16_t instr);
        void handleMovCmpAddSub(uint16_t instr);
        void handleUncondBranch(uint16_t instr);
        void handleCondBranch(uint16_t instr);
        void handleAddSub(uint16_t instr);
        void handleShiftImmediate(uint16_t instr);

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

        void BXWritePC(uint32_t address);
        void handleMisc(uint16_t instr);
        void handleAddr(uint16_t instr);
        void handleLoadStoreReg(uint16_t instr);
        void handleSpRelative(uint16_t instr);
        void decode(void);
        void print_state(void) const;

    private:

};

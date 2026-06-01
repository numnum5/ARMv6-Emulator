// ARMv6 Cycle-Accurate Emulator Core

export const CPSR_N = 1 << 31; // Negative
export const CPSR_Z = 1 << 30; // Zero
export const CPSR_C = 1 << 29; // Carry
export const CPSR_V = 1 << 28; // Overflow
export const CPSR_T = 1 << 5;  // Thumb mode
export const CPSR_I = 1 << 7;  // IRQ disable
export const CPSR_F = 1 << 6;  // FIQ disable

export type ARMMode = 'ARM' | 'THUMB';

export interface CPUState {
  // General purpose registers R0-R15
  regs: Uint32Array;
  // Current Program Status Register
  cpsr: number;
  // Saved PSR (banked)
  spsr: number;
  // Cycle counter
  cycles: number;
  // Pipeline state
  pipeline: PipelineState;
  // Halted flag
  halted: boolean;
  // Exception flag
  exception: string | null;
}

export interface PipelineState {
  fetch: number | null;
  decode: number | null;
  execute: number | null;
  fetchAddr: number;
  decodeAddr: number;
  executeAddr: number;
}

// Sub-cycle stepping: each instruction passes through FETCH → DECODE → EXECUTE
export type MicroStage = 'FETCH' | 'DECODE' | 'EXECUTE';

export interface SubCycleState {
  stage: MicroStage;
  pendingInstr: number | null;
  pendingAddr: number;
  pendingDecoded: DecodedInstruction | null;
  microCycle: number; // cycle within current instruction
}

export interface DecodedInstruction {
  addr: number;
  raw: number;
  mnemonic: string;
  operands: string;
  cycles: number;
  type: InstructionType;
}

export type InstructionType =
  | 'DATA_PROCESSING'
  | 'MULTIPLY'
  | 'LOAD_STORE'
  | 'BRANCH'
  | 'COPROCESSOR'
  | 'UNDEFINED'
  | 'SWI'
  | 'NOP';

export const CONDITION_CODES = ['EQ','NE','CS','CC','MI','PL','VS','VC','HI','LS','GE','LT','GT','LE','AL','NV'];

export const REG_NAMES = ['R0','R1','R2','R3','R4','R5','R6','R7','R8','R9','R10','R11','R12','SP','LR','PC'];

const MEM_SIZE = 0x100000; // 1MB

export class ARMv6CPU {
  regs: Uint32Array;
  cpsr: number;
  spsr: number;
  cycles: number;
  memory: Uint8Array;
  halted: boolean;
  exception: string | null;
  pipeline: PipelineState;
  history: DecodedInstruction[];
  breakpoints: Set<number>;
  watchpoints: Set<number>;
  cycleLog: { cycle: number; addr: number; mnemonic: string; stage: MicroStage }[];
  subCycle: SubCycleState;

  constructor() {
    this.regs = new Uint32Array(16);
    this.cpsr = 0x10; // User mode
    this.spsr = 0;
    this.cycles = 0;
    this.memory = new Uint8Array(MEM_SIZE);
    this.halted = false;
    this.exception = null;
    this.history = [];
    this.breakpoints = new Set();
    this.watchpoints = new Set();
    this.cycleLog = [];
    this.pipeline = {
      fetch: null, decode: null, execute: null,
      fetchAddr: 0, decodeAddr: 0, executeAddr: 0,
    };
    this.subCycle = { stage: 'FETCH', pendingInstr: null, pendingAddr: 0, pendingDecoded: null, microCycle: 0 };
    this.reset();
  }

  reset() {
    this.regs.fill(0);
    this.regs[13] = 0x8000;  // SP
    this.regs[15] = 0x0000;  // PC
    this.cpsr = 0xD3;        // SVC mode, IRQ/FIQ disabled
    this.cycles = 0;
    this.halted = false;
    this.exception = null;
    this.history = [];
    this.cycleLog = [];
    this.pipeline = {
      fetch: null, decode: null, execute: null,
      fetchAddr: 0, decodeAddr: 0, executeAddr: 0,
    };
    this.subCycle = { stage: 'FETCH', pendingInstr: null, pendingAddr: 0, pendingDecoded: null, microCycle: 0 };
  }

  get pc(): number { return this.regs[15]; }
  set pc(v: number) { this.regs[15] = v >>> 0; }

  get sp(): number { return this.regs[13]; }
  get lr(): number { return this.regs[14]; }

  isThumb(): boolean { return (this.cpsr & CPSR_T) !== 0; }

  getMode(): ARMMode { return this.isThumb() ? 'THUMB' : 'ARM'; }

  // Flag helpers
  getN(): boolean { return (this.cpsr & CPSR_N) !== 0; }
  getZ(): boolean { return (this.cpsr & CPSR_Z) !== 0; }
  getC(): boolean { return (this.cpsr & CPSR_C) !== 0; }
  getV(): boolean { return (this.cpsr & CPSR_V) !== 0; }

  setN(v: boolean) { v ? (this.cpsr |= CPSR_N) : (this.cpsr &= ~CPSR_N); }
  setZ(v: boolean) { v ? (this.cpsr |= CPSR_Z) : (this.cpsr &= ~CPSR_Z); }
  setC(v: boolean) { v ? (this.cpsr |= CPSR_C) : (this.cpsr &= ~CPSR_C); }
  setV(v: boolean) { v ? (this.cpsr |= CPSR_V) : (this.cpsr &= ~CPSR_V); }

  setFlags(result: number, carry?: boolean, overflow?: boolean) {
    this.setN((result & 0x80000000) !== 0);
    this.setZ((result >>> 0) === 0);
    if (carry !== undefined) this.setC(carry);
    if (overflow !== undefined) this.setV(overflow);
  }

  checkCondition(cond: number): boolean {
    switch (cond) {
      case 0x0: return this.getZ();                              // EQ
      case 0x1: return !this.getZ();                            // NE
      case 0x2: return this.getC();                             // CS
      case 0x3: return !this.getC();                            // CC
      case 0x4: return this.getN();                             // MI
      case 0x5: return !this.getN();                            // PL
      case 0x6: return this.getV();                             // VS
      case 0x7: return !this.getV();                            // VC
      case 0x8: return this.getC() && !this.getZ();             // HI
      case 0x9: return !this.getC() || this.getZ();             // LS
      case 0xA: return this.getN() === this.getV();             // GE
      case 0xB: return this.getN() !== this.getV();             // LT
      case 0xC: return !this.getZ() && (this.getN() === this.getV()); // GT
      case 0xD: return this.getZ() || (this.getN() !== this.getV()); // LE
      case 0xE: return true;                                    // AL
      default: return false;                                    // NV
    }
  }

  // Memory access
  read32(addr: number): number {
    addr = addr & (MEM_SIZE - 1);
    return (
      this.memory[addr] |
      (this.memory[addr + 1] << 8) |
      (this.memory[addr + 2] << 16) |
      (this.memory[addr + 3] << 24)
    ) >>> 0;
  }

  read16(addr: number): number {
    addr = addr & (MEM_SIZE - 1);
    return (this.memory[addr] | (this.memory[addr + 1] << 8)) & 0xFFFF;
  }

  read8(addr: number): number {
    return this.memory[addr & (MEM_SIZE - 1)];
  }

  write32(addr: number, val: number) {
    addr = addr & (MEM_SIZE - 1);
    val = val >>> 0;
    this.memory[addr] = val & 0xFF;
    this.memory[addr + 1] = (val >> 8) & 0xFF;
    this.memory[addr + 2] = (val >> 16) & 0xFF;
    this.memory[addr + 3] = (val >> 24) & 0xFF;
  }

  write8(addr: number, val: number) {
    this.memory[addr & (MEM_SIZE - 1)] = val & 0xFF;
  }

  // Barrel shifter
  applyShift(val: number, shiftType: number, shiftAmt: number): { result: number; carry: boolean } {
    if (shiftAmt === 0) return { result: val >>> 0, carry: this.getC() };
    switch (shiftType) {
      case 0: // LSL
        return { result: (val << shiftAmt) >>> 0, carry: ((val >>> (32 - shiftAmt)) & 1) !== 0 };
      case 1: // LSR
        return { result: (val >>> shiftAmt) >>> 0, carry: ((val >>> (shiftAmt - 1)) & 1) !== 0 };
      case 2: // ASR
        return { result: (val >> shiftAmt) >>> 0, carry: ((val >>> (shiftAmt - 1)) & 1) !== 0 };
      case 3: // ROR
        return { result: ((val >>> shiftAmt) | (val << (32 - shiftAmt))) >>> 0, carry: ((val >>> (shiftAmt - 1)) & 1) !== 0 };
      default:
        return { result: val >>> 0, carry: this.getC() };
    }
  }

  // Decode ARM instruction to human-readable form
  decodeInstruction(instr: number, addr: number): DecodedInstruction {
    const cond = (instr >>> 28) & 0xF; // reserved for future condition check
    const condStr = cond === 0xE ? '' : CONDITION_CODES[cond];

    // Branches
    if ((instr & 0x0E000000) === 0x0A000000) {
      const link = (instr & (1 << 24)) !== 0;
      const offset = ((instr & 0xFFFFFF) << 8) >> 8; // sign extend
      const target = (addr + 8 + offset * 4) >>> 0;
      return {
        addr, raw: instr,
        mnemonic: `B${link ? 'L' : ''}${condStr}`,
        operands: `#0x${target.toString(16).toUpperCase().padStart(8,'0')}`,
        cycles: link ? 3 : 3,
        type: 'BRANCH',
      };
    }

    // Data processing
    if ((instr & 0x0C000000) === 0x00000000) {
      const opcode = (instr >>> 21) & 0xF;
      const S = (instr & (1 << 20)) !== 0;
      const Rn = (instr >>> 16) & 0xF;
      const Rd = (instr >>> 12) & 0xF;
      const ops = ['AND','EOR','SUB','RSB','ADD','ADC','SBC','RSC','TST','TEQ','CMP','CMN','ORR','MOV','BIC','MVN'];
      const mnem = `${ops[opcode]}${condStr}${S ? 'S' : ''}`;
      const imm = (instr & (1 << 25)) !== 0;
      let op2 = '';
      if (imm) {
        const rot = ((instr >>> 8) & 0xF) * 2;
        const imm8 = instr & 0xFF;
        const val = ((imm8 >>> rot) | (imm8 << (32 - rot))) >>> 0;
        op2 = `#0x${val.toString(16).toUpperCase()}`;
      } else {
        const Rm = instr & 0xF;
        const shiftType = (instr >>> 5) & 0x3;
        const shiftStr = ['LSL','LSR','ASR','ROR'][shiftType];
        const shiftAmt = (instr >>> 7) & 0x1F;
        op2 = shiftAmt ? `${REG_NAMES[Rm]}, ${shiftStr} #${shiftAmt}` : REG_NAMES[Rm];
      }
      const noRn = opcode >= 13; // MOV, MVN
      const noDst = opcode >= 8 && opcode <= 11; // TST,TEQ,CMP,CMN
      let operands = '';
      if (noDst) operands = `${REG_NAMES[Rn]}, ${op2}`;
      else if (noRn) operands = `${REG_NAMES[Rd]}, ${op2}`;
      else operands = `${REG_NAMES[Rd]}, ${REG_NAMES[Rn]}, ${op2}`;
      return { addr, raw: instr, mnemonic: mnem, operands, cycles: 1, type: 'DATA_PROCESSING' };
    }

    // LDR/STR
    if ((instr & 0x0C000000) === 0x04000000) {
      const load = (instr & (1 << 20)) !== 0;
      const byte = (instr & (1 << 22)) !== 0;
      const Rn = (instr >>> 16) & 0xF;
      const Rd = (instr >>> 12) & 0xF;
      const imm = !(instr & (1 << 25));
      const offset = imm ? (instr & 0xFFF) : (instr & 0xF);
      const mnem = `${load ? 'LDR' : 'STR'}${condStr}${byte ? 'B' : ''}`;
      const operands = `${REG_NAMES[Rd]}, [${REG_NAMES[Rn]}, #${offset}]`;
      return { addr, raw: instr, mnemonic: mnem, operands, cycles: load ? 3 : 2, type: 'LOAD_STORE' };
    }

    // MUL
    if ((instr & 0x0FC000F0) === 0x00000090) {
      const Rd = (instr >>> 16) & 0xF;
      const Rs = (instr >>> 8) & 0xF;
      const Rm = instr & 0xF;
      return {
        addr, raw: instr,
        mnemonic: `MUL${condStr}`,
        operands: `${REG_NAMES[Rd]}, ${REG_NAMES[Rm]}, ${REG_NAMES[Rs]}`,
        cycles: 4, type: 'MULTIPLY',
      };
    }

    // SWI
    if ((instr & 0x0F000000) === 0x0F000000) {
      const imm = instr & 0xFFFFFF;
      return { addr, raw: instr, mnemonic: `SWI${condStr}`, operands: `#0x${imm.toString(16)}`, cycles: 3, type: 'SWI' };
    }

    return { addr, raw: instr, mnemonic: 'UNDEF', operands: `0x${instr.toString(16).toUpperCase().padStart(8,'0')}`, cycles: 1, type: 'UNDEFINED' };
  }

  // Execute one ARM instruction cycle
  step(): DecodedInstruction | null {
    if (this.halted) return null;

    const addr = this.pc;
    const instr = this.read32(addr);
    this.pc = (addr + 4) >>> 0;

    const decoded = this.decodeInstruction(instr, addr);
    const cond = (instr >>> 28) & 0xF;

    if (!this.checkCondition(cond)) {
      this.cycles += 1; // 1 cycle even if skipped (pipeline)
      decoded.mnemonic = `[${decoded.mnemonic}]`;
    } else {
      this.executeInstruction(instr, decoded);
      this.cycles += decoded.cycles;
    }

    // Update pipeline visual
    this.pipeline.execute = this.pipeline.decode;
    this.pipeline.executeAddr = this.pipeline.decodeAddr;
    this.pipeline.decode = this.pipeline.fetch;
    this.pipeline.decodeAddr = this.pipeline.fetchAddr;
    this.pipeline.fetch = this.read32(this.pc);
    this.pipeline.fetchAddr = this.pc;

    this.history.unshift(decoded);
    if (this.history.length > 64) this.history.pop();

    this.cycleLog.push({ cycle: this.cycles, addr: decoded.addr, mnemonic: decoded.mnemonic, stage: 'EXECUTE' });
    if (this.cycleLog.length > 256) this.cycleLog.shift();

    return decoded;
  }

  // Sub-cycle step: advances one pipeline micro-stage at a time (FETCH → DECODE → EXECUTE)
  // Returns the stage that just completed, and the decoded instruction once EXECUTE completes
  microStep(): { stage: MicroStage; decoded: DecodedInstruction | null } {
    if (this.halted) return { stage: 'EXECUTE', decoded: null };

    const sc = this.subCycle;

    if (sc.stage === 'FETCH') {
      sc.pendingAddr = this.pc;
      sc.pendingInstr = this.read32(this.pc);
      this.pc = (this.pc + 4) >>> 0;
      this.cycles += 1;
      this.cycleLog.push({ cycle: this.cycles, addr: sc.pendingAddr, mnemonic: '...FETCH', stage: 'FETCH' });
      if (this.cycleLog.length > 256) this.cycleLog.shift();
      // Update pipeline fetch stage
      this.pipeline.fetch = sc.pendingInstr;
      this.pipeline.fetchAddr = sc.pendingAddr;
      sc.stage = 'DECODE';
      return { stage: 'FETCH', decoded: null };
    }

    if (sc.stage === 'DECODE') {
      sc.pendingDecoded = this.decodeInstruction(sc.pendingInstr!, sc.pendingAddr);
      this.cycles += 1;
      this.cycleLog.push({ cycle: this.cycles, addr: sc.pendingAddr, mnemonic: `...DECODE ${sc.pendingDecoded.mnemonic}`, stage: 'DECODE' });
      if (this.cycleLog.length > 256) this.cycleLog.shift();
      // Advance pipeline
      this.pipeline.decode = this.pipeline.fetch;
      this.pipeline.decodeAddr = this.pipeline.fetchAddr;
      sc.stage = 'EXECUTE';
      return { stage: 'DECODE', decoded: null };
    }

    // EXECUTE
    const decoded = sc.pendingDecoded!;
    const instr   = sc.pendingInstr!;
    const cond    = (instr >>> 28) & 0xF;

    if (!this.checkCondition(cond)) {
      decoded.mnemonic = `[${decoded.mnemonic}]`;
    } else {
      this.executeInstruction(instr, decoded);
    }
    this.cycles += 1;

    // Advance pipeline
    this.pipeline.execute = this.pipeline.decode;
    this.pipeline.executeAddr = this.pipeline.decodeAddr;

    this.history.unshift(decoded);
    if (this.history.length > 64) this.history.pop();

    this.cycleLog.push({ cycle: this.cycles, addr: decoded.addr, mnemonic: decoded.mnemonic, stage: 'EXECUTE' });
    if (this.cycleLog.length > 256) this.cycleLog.shift();

    // Reset for next instruction
    sc.stage = 'FETCH';
    sc.pendingInstr = null;
    sc.pendingDecoded = null;
    sc.microCycle = 0;

    return { stage: 'EXECUTE', decoded };
  }

  executeInstruction(instr: number, decoded: DecodedInstruction) {
    // Branch
    if (decoded.type === 'BRANCH') {
      const link = (instr & (1 << 24)) !== 0;
      const offset = ((instr & 0xFFFFFF) << 8) >> 8;
      const target = (this.pc - 4 + 4 + offset * 4) >>> 0;
      if (link) this.regs[14] = (this.pc - 4) >>> 0;
      this.pc = target;
      return;
    }

    // SWI
    if (decoded.type === 'SWI') {
      this.exception = `SWI #${instr & 0xFFFFFF}`;
      this.halted = true;
      return;
    }

    // MUL
    if (decoded.type === 'MULTIPLY') {
      const Rd = (instr >>> 16) & 0xF;
      const Rs = (instr >>> 8) & 0xF;
      const Rm = instr & 0xF;
      this.regs[Rd] = Math.imul(this.regs[Rm], this.regs[Rs]) >>> 0;
      return;
    }

    // LDR/STR
    if (decoded.type === 'LOAD_STORE') {
      const load = (instr & (1 << 20)) !== 0;
      const byte = (instr & (1 << 22)) !== 0;
      const Rn = (instr >>> 16) & 0xF;
      const Rd = (instr >>> 12) & 0xF;
      const immOff = !(instr & (1 << 25));
      const U = (instr & (1 << 23)) !== 0;
      const offset = immOff ? (instr & 0xFFF) : (instr & 0xF);
      const addr = (U ? this.regs[Rn] + offset : this.regs[Rn] - offset) >>> 0;
      if (load) {
        this.regs[Rd] = byte ? this.read8(addr) : this.read32(addr);
      } else {
        if (byte) this.write8(addr, this.regs[Rd] & 0xFF);
        else this.write32(addr, this.regs[Rd]);
      }
      return;
    }

    // Data Processing
    if (decoded.type === 'DATA_PROCESSING') {
      const opcode = (instr >>> 21) & 0xF;
      const S = (instr & (1 << 20)) !== 0;
      const Rn = (instr >>> 16) & 0xF;
      const Rd = (instr >>> 12) & 0xF;
      const useImm = (instr & (1 << 25)) !== 0;

      let op2: number;
      let shiftCarry = this.getC();

      if (useImm) {
        const rot = ((instr >>> 8) & 0xF) * 2;
        const imm8 = instr & 0xFF;
        op2 = ((imm8 >>> rot) | (imm8 << (32 - rot))) >>> 0;
      } else {
        const Rm = instr & 0xF;
        const shiftType = (instr >>> 5) & 0x3;
        const shiftAmt = (instr >>> 7) & 0x1F;
        const shifted = this.applyShift(this.regs[Rm], shiftType, shiftAmt);
        op2 = shifted.result;
        shiftCarry = shifted.carry;
      }

      const rn = this.regs[Rn];
      let result = 0;
      let carry = shiftCarry;
      let overflow = this.getV();

      switch (opcode) {
        case 0x0: result = (rn & op2) >>> 0; break; // AND
        case 0x1: result = (rn ^ op2) >>> 0; break; // EOR
        case 0x2: { const r = rn - op2; carry = rn >= op2; overflow = ((rn ^ op2) & (rn ^ r)) >>> 31 !== 0; result = r >>> 0; break; } // SUB
        case 0x3: { const r = op2 - rn; carry = op2 >= rn; result = r >>> 0; break; } // RSB
        case 0x4: { const r = rn + op2; carry = r > 0xFFFFFFFF; overflow = !((rn ^ op2) & 0x80000000) && ((rn ^ r) & 0x80000000) !== 0; result = r >>> 0; break; } // ADD
        case 0x5: { const r = rn + op2 + (this.getC() ? 1 : 0); carry = r > 0xFFFFFFFF; result = r >>> 0; break; } // ADC
        case 0x6: { const r = rn - op2 + (this.getC() ? 1 : 0) - 1; carry = r >= 0; result = r >>> 0; break; } // SBC
        case 0x7: { const r = op2 - rn + (this.getC() ? 1 : 0) - 1; result = r >>> 0; break; } // RSC
        case 0x8: result = (rn & op2) >>> 0; break; // TST (no writeback)
        case 0x9: result = (rn ^ op2) >>> 0; break; // TEQ
        case 0xA: { const r = rn - op2; carry = rn >= op2; result = r >>> 0; break; } // CMP
        case 0xB: { const r = rn + op2; carry = r > 0xFFFFFFFF; result = r >>> 0; break; } // CMN
        case 0xC: result = (rn | op2) >>> 0; break; // ORR
        case 0xD: result = op2; carry = shiftCarry; break; // MOV
        case 0xE: result = (rn & ~op2) >>> 0; break; // BIC
        case 0xF: result = (~op2) >>> 0; break; // MVN
      }

      const hasWriteback = opcode < 0x8 || opcode > 0xB;
      if (hasWriteback) this.regs[Rd] = result;
      if (S) this.setFlags(result, carry, overflow);
    }
  }

  // Load a program into memory
  loadProgram(bytes: Uint8Array, startAddr = 0) {
    for (let i = 0; i < bytes.length && startAddr + i < MEM_SIZE; i++) {
      this.memory[startAddr + i] = bytes[i];
    }
    this.pc = startAddr;
  }

  getState(): CPUState {
    return {
      regs: new Uint32Array(this.regs),
      cpsr: this.cpsr,
      spsr: this.spsr,
      cycles: this.cycles,
      pipeline: { ...this.pipeline },
      halted: this.halted,
      exception: this.exception,
    };
  }
}

// Sample programs
export const SAMPLE_PROGRAMS: Record<string, { name: string; bytes: number[] }> = {
  fibonacci: {
    name: 'Fibonacci',
    bytes: [
      // MOV R0, #0         (fib[0] = 0)
      0x00, 0x00, 0xA0, 0xE3,
      // MOV R1, #1         (fib[1] = 1)
      0x01, 0x10, 0xA0, 0xE3,
      // MOV R2, #10        (counter = 10)
      0x0A, 0x20, 0xA0, 0xE3,
      // loop: ADD R3, R0, R1
      0x01, 0x30, 0x80, 0xE0,
      // MOV R0, R1
      0x01, 0x00, 0xA0, 0xE1,
      // MOV R1, R3
      0x03, 0x10, 0xA0, 0xE1,
      // SUBS R2, R2, #1
      0x01, 0x20, 0x52, 0xE2,
      // BNE loop (-4 words back)
      0xFB, 0xFF, 0xFF, 0x1A,
      // B . (halt)
      0xFE, 0xFF, 0xFF, 0xEA,
    ],
  },
  memcopy: {
    name: 'Memory Copy',
    bytes: [
      // MOV R0, #0x100     (src)
      0x01, 0x05, 0xA0, 0xE3,
      // MOV R1, #0x200     (dst)
      0x02, 0x1A, 0xA0, 0xE3,
      // MOV R2, #8         (count)
      0x08, 0x20, 0xA0, 0xE3,
      // loop: LDR R3, [R0], #4
      0x04, 0x30, 0x90, 0xE4,
      // STR R3, [R1], #4
      0x04, 0x30, 0x81, 0xE4,
      // SUBS R2, R2, #1
      0x01, 0x20, 0x52, 0xE2,
      // BNE loop
      0xFB, 0xFF, 0xFF, 0x1A,
      // B .
      0xFE, 0xFF, 0xFF, 0xEA,
    ],
  },
  counter: {
    name: 'Counter',
    bytes: [
      // MOV R0, #0
      0x00, 0x00, 0xA0, 0xE3,
      // loop: ADD R0, R0, #1
      0x01, 0x00, 0x80, 0xE2,
      // CMP R0, #20
      0x14, 0x00, 0x50, 0xE3,
      // BNE loop
      0xFD, 0xFF, 0xFF, 0x1A,
      // B .
      0xFE, 0xFF, 0xFF, 0xEA,
    ],
  },
};

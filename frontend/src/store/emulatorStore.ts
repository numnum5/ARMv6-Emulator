import { create } from 'zustand';
import type { CPUState, DecodedInstruction, MicroStage } from '../emulator/cpu';
import { ARMv6CPU, REG_NAMES, SAMPLE_PROGRAMS } from '../emulator/cpu';
import { parseELF, loadELFIntoMemory } from '../emulator/elf';

export type RunState = 'stopped' | 'running' | 'paused' | 'halted';
export type StepMode = 'instruction' | 'cycle';
export type NavTab = 'emulator' | 'code' | 'upload';

export interface LoadedBinary {
  name: string;
  type: 'elf' | 'raw' | 'sample';
  entryPoint: number;
  size: number;
  error?: string;
}

interface EmulatorStore {
  cpu: ARMv6CPU;
  cpuState: CPUState;
  runState: RunState;
  stepMode: StepMode;
  activeTab: NavTab;
  lastInstruction: DecodedInstruction | null;
  currentMicroStage: MicroStage | null;
  disassembly: DecodedInstruction[];
  memViewAddr: number;
  selectedProgram: string;
  stepsPerSecond: number;
  intervalId: ReturnType<typeof setInterval> | null;
  loadedBinary: LoadedBinary | null;
  cCode: string;

  // Actions
  reset: () => void;
  step: () => void;           // step one instruction OR one cycle depending on stepMode
  stepInstruction: () => void;
  stepCycle: () => void;
  run: () => void;
  pause: () => void;
  stop: () => void;
  setStepMode: (m: StepMode) => void;
  setActiveTab: (t: NavTab) => void;
  setMemViewAddr: (addr: number) => void;
  loadProgram: (name: string) => void;
  loadELFFile: (file: File) => Promise<void>;
  loadRawBinary: (file: File, entryPoint?: number) => Promise<void>;
  setCCode: (code: string) => void;
  setStepsPerSecond: (v: number) => void;
  toggleBreakpoint: (addr: number) => void;
}

function buildDisassembly(cpu: ARMv6CPU, startAddr: number, count = 24): DecodedInstruction[] {
  const result: DecodedInstruction[] = [];
  for (let i = 0; i < count; i++) {
    const addr = (startAddr + i * 4) & 0xFFFFF;
    result.push(cpu.decodeInstruction(cpu.read32(addr), addr));
  }
  return result;
}

function disasmAnchor(cpu: ARMv6CPU) {
  const pc = cpu.pc;
  return buildDisassembly(cpu, pc > 8 ? pc - 8 : 0);
}

const initialCPU = new ARMv6CPU();
initialCPU.loadProgram(new Uint8Array(SAMPLE_PROGRAMS['fibonacci'].bytes), 0);

const DEFAULT_C_CODE = `// ARMv6 C example — compile with:
// arm-none-eabi-gcc -march=armv6 -O1 -nostdlib -o out.elf main.c
// Then upload the .elf via the Upload tab

int add(int a, int b) {
    return a + b;
}

int fibonacci(int n) {
    if (n <= 1) return n;
    int a = 0, b = 1;
    for (int i = 2; i <= n; i++) {
        int c = a + b;
        a = b;
        b = c;
    }
    return b;
}

void _start(void) {
    int result = fibonacci(10);
    (void)result;
    // SWI to halt
    __asm__("swi #0");
}
`;

export const useEmulatorStore = create<EmulatorStore>((set, get) => ({
  cpu: initialCPU,
  cpuState: initialCPU.getState(),
  runState: 'stopped',
  stepMode: 'instruction',
  activeTab: 'emulator',
  lastInstruction: null,
  currentMicroStage: null,
  disassembly: buildDisassembly(initialCPU, 0),
  memViewAddr: 0,
  selectedProgram: 'fibonacci',
  stepsPerSecond: 4,
  intervalId: null,
  loadedBinary: null,
  cCode: DEFAULT_C_CODE,

  reset: () => {
    const { cpu, intervalId, selectedProgram, loadedBinary } = get();
    if (intervalId) clearInterval(intervalId);
    cpu.reset();
    // Reload whatever was loaded
    if (loadedBinary?.type === 'sample') {
      const prog = SAMPLE_PROGRAMS[selectedProgram];
      if (prog) cpu.loadProgram(new Uint8Array(prog.bytes), 0);
    }
    set({
      cpuState: cpu.getState(), runState: 'stopped',
      lastInstruction: null, currentMicroStage: null,
      disassembly: buildDisassembly(cpu, 0), intervalId: null,
    });
  },

  stepInstruction: () => {
    const { cpu } = get();
    if (cpu.halted) return;
    const decoded = cpu.step();
    set({
      cpuState: cpu.getState(), lastInstruction: decoded,
      currentMicroStage: null,
      disassembly: disasmAnchor(cpu),
    });
  },

  stepCycle: () => {
    const { cpu } = get();
    if (cpu.halted) return;
    const { stage, decoded } = cpu.microStep();
    set({
      cpuState: cpu.getState(),
      currentMicroStage: stage,
      lastInstruction: decoded ?? get().lastInstruction,
      disassembly: decoded ? disasmAnchor(cpu) : get().disassembly,
    });
  },

  step: () => {
    const { stepMode } = get();
    if (stepMode === 'cycle') get().stepCycle();
    else get().stepInstruction();
  },

  run: () => {
    const old = get().intervalId;
    if (old) clearInterval(old);
    const id = setInterval(() => {
      const store = get();
      if (store.cpu.halted) {
        clearInterval(id);
        set({ runState: 'halted', intervalId: null, cpuState: store.cpu.getState() });
        return;
      }
      const c = store.cpu;
      const iters = Math.max(1, Math.floor(store.stepsPerSecond / 10));
      for (let i = 0; i < iters; i++) {
        if (store.stepMode === 'cycle') {
          c.microStep();
        } else {
          c.step();
        }
        if (c.halted || c.breakpoints.has(c.pc)) break;
      }
      set({
        cpuState: c.getState(),
        lastInstruction: c.history[0] ?? null,
        disassembly: disasmAnchor(c),
      });
    }, 100);
    set({ runState: 'running', intervalId: id });
  },

  pause: () => {
    const { intervalId } = get();
    if (intervalId) clearInterval(intervalId);
    set({ runState: 'paused', intervalId: null });
  },

  stop: () => {
    const { intervalId, cpu, selectedProgram } = get();
    if (intervalId) clearInterval(intervalId);
    cpu.reset();
    const prog = SAMPLE_PROGRAMS[selectedProgram];
    if (prog) cpu.loadProgram(new Uint8Array(prog.bytes), 0);
    set({
      cpuState: cpu.getState(), runState: 'stopped',
      lastInstruction: null, currentMicroStage: null,
      disassembly: buildDisassembly(cpu, 0), intervalId: null,
    });
  },

  setStepMode: (m) => set({ stepMode: m }),
  setActiveTab: (t) => set({ activeTab: t }),
  setMemViewAddr: (addr) => set({ memViewAddr: addr }),
  setCCode: (code) => set({ cCode: code }),
  setStepsPerSecond: (v) => set({ stepsPerSecond: v }),

  loadProgram: (name) => {
    const { cpu, intervalId } = get();
    if (intervalId) clearInterval(intervalId);
    const prog = SAMPLE_PROGRAMS[name];
    if (!prog) return;
    cpu.reset();
    cpu.loadProgram(new Uint8Array(prog.bytes), 0);
    set({
      selectedProgram: name,
      loadedBinary: { name: prog.name, type: 'sample', entryPoint: 0, size: prog.bytes.length },
      cpuState: cpu.getState(), runState: 'stopped',
      lastInstruction: null, currentMicroStage: null,
      disassembly: buildDisassembly(cpu, 0), intervalId: null,
    });
  },

  loadELFFile: async (file) => {
    const { cpu, intervalId } = get();
    if (intervalId) clearInterval(intervalId);
    const buf = await file.arrayBuffer();
    const bytes = new Uint8Array(buf);
    const elf = parseELF(bytes);
    if (!elf.valid) {
      set({ loadedBinary: { name: file.name, type: 'elf', entryPoint: 0, size: 0, error: elf.error } });
      return;
    }
    cpu.reset();
    const entry = loadELFIntoMemory(elf, cpu.memory, 0x100000);
    cpu.pc = entry;
    cpu.regs[13] = 0x80000; // SP
    set({
      loadedBinary: { name: file.name, type: 'elf', entryPoint: entry, size: bytes.length },
      cpuState: cpu.getState(), runState: 'stopped',
      lastInstruction: null, currentMicroStage: null,
      disassembly: buildDisassembly(cpu, entry), intervalId: null,
      memViewAddr: entry & ~0xF,
    });
  },

  loadRawBinary: async (file, entryPoint = 0) => {
    const { cpu, intervalId } = get();
    if (intervalId) clearInterval(intervalId);
    const buf = await file.arrayBuffer();
    const bytes = new Uint8Array(buf);
    cpu.reset();
    cpu.loadProgram(bytes, entryPoint);
    set({
      loadedBinary: { name: file.name, type: 'raw', entryPoint, size: bytes.length },
      cpuState: cpu.getState(), runState: 'stopped',
      lastInstruction: null, currentMicroStage: null,
      disassembly: buildDisassembly(cpu, entryPoint), intervalId: null,
      memViewAddr: entryPoint & ~0xF,
    });
  },

  toggleBreakpoint: (addr) => {
    const { cpu } = get();
    if (cpu.breakpoints.has(addr)) cpu.breakpoints.delete(addr);
    else cpu.breakpoints.add(addr);
    set({ disassembly: disasmAnchor(cpu) });
  },
}));

export { REG_NAMES };

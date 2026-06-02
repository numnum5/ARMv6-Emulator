import { create } from 'zustand';
import type { MicroStage } from '../emulator/cpu';
import { ARMv6CPU, REG_NAMES, SAMPLE_PROGRAMS } from '../emulator/cpu';

export type RunState = 'stopped' | 'running' | 'paused' | 'halted';
export type StepMode = 'instruction' | 'cycle';
export type NavTab   = 'emulator' | 'code' | 'upload' | 'settings';
export type WsStatus = 'disconnected' | 'connecting' | 'connected' | 'error';

// Shape all panels use — covers both local CPU and backend responses
export interface CpuState {
  regs:    number[];        // R0–R15 (16 elements)
  cpsr:    number;
  cycles:  number;          // total cycles / clock ticks
  halted:  boolean;
  exception: string | null;
  pipeline: {
    fetch:       number | null;
    decode:      number | null;
    execute:     number | null;
    fetchAddr:   number;
    decodeAddr:  number;
    executeAddr: number;
  };
}

export interface LoadedBinary {
  name: string;
  type: 'elf' | 'raw' | 'sample';
  entryPoint: number;
  size: number;
  error?: string;
}

const DEFAULT_C_CODE = `// ARMv6 C example — compile with:
// arm-none-eabi-gcc -march=armv6 -O1 -nostdlib -Ttext=0x0 -o out.elf main.c
// Then upload the .elf via the Upload tab

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
    __asm__("swi #0");
}
`;

// ─── Local JS CPU (used when no backend connected) ────────────────────────────
const localCPU = new ARMv6CPU();
localCPU.loadProgram(new Uint8Array(SAMPLE_PROGRAMS['fibonacci'].bytes), 0);

function localCpuState(): CpuState {
  return {
    regs:      Array.from(localCPU.regs),
    cpsr:      localCPU.cpsr,
    cycles:    localCPU.cycles,
    halted:    localCPU.halted,
    exception: localCPU.exception,
    pipeline:  { ...localCPU.pipeline },
  };
}

function buildDisassembly(target: ARMv6CPU, startAddr: number, count = 24) {
  return Array.from({ length: count }, (_, i) => {
    const addr = (startAddr + i * 4) & 0xFFFFF;
    return target.decodeInstruction(target.read32(addr), addr);
  });
}

// ─── Store interface ──────────────────────────────────────────────────────────
interface EmulatorStore {
  // State panels read
  cpuState:          CpuState;
  displayState:      CpuState;   // alias — both point to same data
  runState:          RunState;
  stepMode:          StepMode;
  activeTab:         NavTab;
  currentMicroStage: MicroStage | null;
  disassembly:       ReturnType<typeof buildDisassembly>;
  memViewAddr:       number;
  cCode:             string;
  loadedBinary:      LoadedBinary | null;
  selectedProgram:   string;
  stepsPerSecond:    number;

  // Local JS CPU (for disassembly + memory view)
  cpu:               ARMv6CPU;
  localCPU:          ARMv6CPU;

  // Local-only actions (used when backend not connected)
  step:           () => void;
  stepInstruction:() => void;
  stepCycle:      () => void;
  run:            () => void;
  pause:          () => void;
  stop:           () => void;
  reset:          () => void;
  loadProgram:    (name: string) => void;

  // UI
  setStepMode:      (m: StepMode) => void;
  setActiveTab:     (t: NavTab) => void;
  setMemViewAddr:   (addr: number) => void;
  setCCode:         (code: string) => void;
  setStepsPerSecond:(v: number) => void;
  setLoadedBinary:  (b: LoadedBinary | null) => void;
  toggleBreakpoint: (addr: number) => void;

  // WS status (set by Toolbar)
  wsStatus:      'disconnected' | 'connecting' | 'connected' | 'error';
  setWsStatus:   (s: 'disconnected' | 'connecting' | 'connected' | 'error') => void;

  // Called by Toolbar when backend sends state
  applyBackendState:(raw: Record<string, unknown>) => void;

  // Internal run interval
  _intervalId: ReturnType<typeof setInterval> | null;
}

// ─── Store ───────────────────────────────────────────────────────────────────
export const useEmulatorStore = create<EmulatorStore>((set, get) => {

  function syncState(target: ARMv6CPU) {
    const s = localCpuState();
    const disassembly = buildDisassembly(target, target.pc > 8 ? target.pc - 8 : 0);
    set({ cpuState: s, displayState: s, disassembly });
  }

  return {
    cpuState:          localCpuState(),
    displayState:      localCpuState(),
    runState:          'stopped',
    stepMode:          'instruction',
    activeTab:         'emulator',
    currentMicroStage: null,
    disassembly:       buildDisassembly(localCPU, 0),
    memViewAddr:       0,
    cCode:             DEFAULT_C_CODE,
    loadedBinary:      null,
    selectedProgram:   'fibonacci',
    stepsPerSecond:    4,
    cpu:               localCPU,
    localCPU,
    _intervalId:       null,
    wsStatus:          'disconnected' as const,
    setWsStatus:       (s) => set({ wsStatus: s }),

    // ── Local step / run ────────────────────────────────────────────────────

    stepInstruction: () => {
      if (localCPU.halted) return;
      localCPU.step();
      syncState(localCPU);
    },

    stepCycle: () => {
      if (localCPU.halted) return;
      const { stage } = localCPU.microStep();
      set({ currentMicroStage: stage });
      syncState(localCPU);
    },

    step: () => {
      const { stepMode } = get();
      if (stepMode === 'cycle') get().stepCycle();
      else get().stepInstruction();
    },

    run: () => {
      const old = get()._intervalId;
      if (old) clearInterval(old);
      const id = setInterval(() => {
        const { stepsPerSecond, stepMode } = get();
        if (localCPU.halted) {
          clearInterval(id);
          syncState(localCPU);
          set({ runState: 'halted', _intervalId: null });
          return;
        }
        const iters = Math.max(1, Math.floor(stepsPerSecond / 10));
        for (let i = 0; i < iters; i++) {
          if (stepMode === 'cycle') localCPU.microStep();
          else localCPU.step();
          if (localCPU.halted || localCPU.breakpoints.has(localCPU.pc)) break;
        }
        syncState(localCPU);
      }, 100);
      set({ runState: 'running', _intervalId: id });
    },

    pause: () => {
      const { _intervalId } = get();
      if (_intervalId) clearInterval(_intervalId);
      set({ runState: 'paused', _intervalId: null });
    },

    stop: () => {
      const { _intervalId, selectedProgram } = get();
      if (_intervalId) clearInterval(_intervalId);
      localCPU.reset();
      const prog = SAMPLE_PROGRAMS[selectedProgram];
      if (prog) localCPU.loadProgram(new Uint8Array(prog.bytes), 0);
      syncState(localCPU);
      set({ runState: 'stopped', _intervalId: null, currentMicroStage: null });
    },

    reset: () => {
      const { _intervalId, selectedProgram } = get();
      if (_intervalId) clearInterval(_intervalId);
      localCPU.reset();
      const prog = SAMPLE_PROGRAMS[selectedProgram];
      if (prog) localCPU.loadProgram(new Uint8Array(prog.bytes), 0);
      syncState(localCPU);
      set({ runState: 'stopped', _intervalId: null, currentMicroStage: null });
    },

    loadProgram: (name) => {
      const { _intervalId } = get();
      if (_intervalId) clearInterval(_intervalId);
      const prog = SAMPLE_PROGRAMS[name];
      if (!prog) return;
      localCPU.reset();
      localCPU.loadProgram(new Uint8Array(prog.bytes), 0);
      syncState(localCPU);
      set({
        selectedProgram: name,
        runState: 'stopped', _intervalId: null, currentMicroStage: null,
        loadedBinary: { name: prog.name, type: 'sample', entryPoint: 0, size: prog.bytes.length },
        disassembly: buildDisassembly(localCPU, 0),
      });
    },

    // ── Backend state injection (called by Toolbar on WS message) ─────────
    applyBackendState: (raw) => {
      const prev = get().cpuState;

      // Map C++ field names → our CpuState shape.
      // C++ currently sends: { pc, cycle }
      // Future fields commented out — uncomment as you add them to glaze:
      const next: CpuState = {
        regs:      (raw.regs as number[] | undefined) ?? prev.regs,
        cpsr:      (raw.cpsr as number  | undefined) ?? prev.cpsr,
        cycles:    (raw.cycle as number | undefined)    // C++ uses "cycle" not "cycles"
                ?? (raw.cycles as number | undefined)
                ?? prev.cycles,
        halted:    (raw.halted as boolean | undefined) ?? prev.halted,
        exception: (raw.exception as string | null | undefined) ?? prev.exception,
        pipeline: {
          fetch:       (raw.fetch       as number | null | undefined) ?? prev.pipeline.fetch,
          decode:      (raw.decode      as number | null | undefined) ?? prev.pipeline.decode,
          execute:     (raw.execute     as number | null | undefined) ?? prev.pipeline.execute,
          fetchAddr:   (raw.fetch_pc    as number | undefined)        // C++ uses fetch_pc
                    ?? (raw.fetchAddr   as number | undefined)
                    ?? prev.pipeline.fetchAddr,
          decodeAddr:  (raw.decodeAddr  as number | undefined) ?? prev.pipeline.decodeAddr,
          executeAddr: (raw.executeAddr as number | undefined) ?? prev.pipeline.executeAddr,
        },
      };

      // Also update local CPU's PC so disassembly follows backend
      const pc = (raw.pc as number | undefined) ?? prev.regs[15];
      localCPU.regs[15] = pc;
      localCPU.cycleLog.push({ cycle: next.cycles, addr: pc, mnemonic: (raw.mnemonic as string) ?? '?', stage: 'EXECUTE' });
      if (localCPU.cycleLog.length > 256) localCPU.cycleLog.shift();

      const disassembly = buildDisassembly(localCPU, pc > 8 ? pc - 8 : 0);

      set({
        cpuState: next,
        displayState: next,
        disassembly,
        runState: next.halted ? 'halted' : get().runState,
      });
    },

    // ── UI setters ──────────────────────────────────────────────────────────
    setStepMode:       (m) => set({ stepMode: m }),
    setActiveTab:      (t) => set({ activeTab: t }),
    setMemViewAddr:    (addr) => set({ memViewAddr: addr }),
    setCCode:          (code) => set({ cCode: code }),
    setStepsPerSecond: (v) => set({ stepsPerSecond: v }),
    setLoadedBinary:   (b) => set({ loadedBinary: b }),

    toggleBreakpoint: (addr) => {
      if (localCPU.breakpoints.has(addr)) localCPU.breakpoints.delete(addr);
      else localCPU.breakpoints.add(addr);
      set({});
    },
  };
});

export { REG_NAMES };

// ─── WS ref exposed for components that need to send commands ────────────────
// This is set by Toolbar on connect and cleared on disconnect.
// Using a module-level ref so it doesn't cause re-renders.
let _wsRef: WebSocket | null = null;
export function setGlobalWs(ws: WebSocket | null) { _wsRef = ws; }
export function getGlobalWs(): WebSocket | null { return _wsRef; }
export function wsSend(msg: string): boolean {
  if (!_wsRef || _wsRef.readyState !== WebSocket.OPEN) return false;
  _wsRef.send(msg);
  return true;
}
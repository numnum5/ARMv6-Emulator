import { useEmulatorStore } from '../store/emulatorStore';
import type { DecodedInstruction } from '../emulator/cpu';

const TYPE_COLOR: Record<string, string> = {
  BRANCH:          'text-[var(--color-neon)]',
  DATA_PROCESSING: 'text-[var(--color-lime)]',
  LOAD_STORE:      'text-[var(--color-amber)]',
  MULTIPLY:        'text-[var(--color-violet)]',
  SWI:             'text-[var(--color-crimson)]',
  UNDEFINED:       'text-[var(--color-crimson)]',
};

export function DisassemblerPanel() {
  const { disassembly, cpuState, toggleBreakpoint, cpu } = useEmulatorStore();
  const pc = cpuState.regs[15];

  return (
    <div className="flex flex-col bg-[var(--color-bg-panel)] border border-[var(--color-border)] rounded-md overflow-hidden min-h-0">
      {/* Header */}
      <div className="flex items-center gap-2 px-3 py-2 bg-[var(--color-bg-card)] border-b border-[var(--color-border)] shrink-0">
        <span className="text-[var(--color-neon)] text-sm">⬡</span>
        <span className="font-[var(--font-display)] text-[11px] font-semibold text-[var(--color-txt-bright)] tracking-widest uppercase">Disassembler</span>
        <div className="ml-auto flex gap-3 text-[9px] text-[var(--color-txt-dim)]">
          <span className="text-[var(--color-neon)]">■ Branch</span>
          <span className="text-[var(--color-lime)]">■ Data</span>
          <span className="text-[var(--color-amber)]">■ Mem</span>
          <span className="text-[var(--color-violet)]">■ Mul</span>
        </div>
      </div>

      {/* Column headers */}
      <div className="grid grid-cols-[14px_90px_76px_96px_1fr_26px] gap-1.5 px-3 py-1 border-b border-[var(--color-border)] bg-[var(--color-bg-card)]/60 text-[9px] text-[var(--color-txt-dim)] tracking-wider uppercase shrink-0">
        <span />
        <span>Addr</span>
        <span>Raw</span>
        <span>Mnemonic</span>
        <span>Operands</span>
        <span className="text-right">Cyc</span>
      </div>

      {/* Instruction list */}
      <div className="overflow-y-auto flex-1">
        {disassembly.map((instr: DecodedInstruction) => {
          const isCurrent = instr.addr === pc - 4 || instr.addr === pc;
          const isBP = cpu.breakpoints.has(instr.addr);
          return (
            <div key={instr.addr}
              onClick={() => toggleBreakpoint(instr.addr)}
              title="Click to toggle breakpoint"
              className={`grid grid-cols-[14px_90px_76px_96px_1fr_26px] gap-1.5 items-center px-3 py-[3px] border-b border-[var(--color-border)]/30 cursor-pointer select-none transition-colors text-[11.5px]
                ${isCurrent
                  ? 'bg-[var(--color-neon)]/6 border-l-2 border-l-[var(--color-neon)]'
                  : 'hover:bg-[var(--color-bg-hover)]'
                }`}>

              {/* Breakpoint dot */}
              <span className={`text-[8px] text-center ${isBP ? 'text-[var(--color-crimson)]' : 'text-[var(--color-border-2)]'}`}>
                {isBP ? '●' : '○'}
              </span>

              {/* Address */}
              <span className={`tabular-nums text-[10px] ${isCurrent ? 'text-[var(--color-neon)]' : 'text-[var(--color-txt-dim)]'}`}>
                {instr.addr.toString(16).toUpperCase().padStart(8, '0')}
              </span>

              {/* Raw bytes */}
              <span className="tabular-nums text-[9.5px] text-[var(--color-border-2)]">
                {instr.raw.toString(16).toUpperCase().padStart(8, '0')}
              </span>

              {/* Mnemonic */}
              <span className={`font-semibold font-[var(--font-display)] text-[12px] tracking-wide ${TYPE_COLOR[instr.type] ?? 'text-[var(--color-txt)]'}`}>
                {instr.mnemonic}
              </span>

              {/* Operands */}
              <span className="text-[var(--color-txt)] truncate">{instr.operands}</span>

              {/* Cycles */}
              <span className="text-right text-[9px] text-[var(--color-txt-dim)]">{instr.cycles}c</span>
            </div>
          );
        })}
      </div>
    </div>
  );
}

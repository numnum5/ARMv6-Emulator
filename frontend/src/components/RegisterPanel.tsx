import { useEmulatorStore, REG_NAMES } from '../store/emulatorStore';
import { CPSR_N, CPSR_Z, CPSR_C, CPSR_V, CPSR_T, CPSR_I, CPSR_F } from '../emulator/cpu';

const FLAGS = [
  { bit: CPSR_N, label: 'N', title: 'Negative' },
  { bit: CPSR_Z, label: 'Z', title: 'Zero' },
  { bit: CPSR_C, label: 'C', title: 'Carry' },
  { bit: CPSR_V, label: 'V', title: 'Overflow' },
  { bit: CPSR_T, label: 'T', title: 'Thumb' },
  { bit: CPSR_I, label: 'I', title: 'IRQ Dis' },
  { bit: CPSR_F, label: 'F', title: 'FIQ Dis' },
];

const REG_ACCENT: Record<number, string> = {
  13: 'bg-[var(--color-amber)]/5 border-l-2 border-l-[var(--color-amber)]',
  14: 'bg-[var(--color-violet)]/5 border-l-2 border-l-[var(--color-violet)]',
  15: 'bg-[var(--color-neon)]/5 border-l-2 border-l-[var(--color-neon)]',
};
const REG_NAME_ACCENT: Record<number, string> = {
  13: 'text-[var(--color-amber)]',
  14: 'text-[var(--color-violet)]',
  15: 'text-[var(--color-neon)]',
};

export function RegisterPanel() {
  const { cpuState } = useEmulatorStore();
  const { regs, cpsr, cycles } = cpuState;

  const mode = (cpsr & 0x1F) === 0x10 ? 'USR' : (cpsr & 0x1F) === 0x13 ? 'SVC'
             : (cpsr & 0x1F) === 0x11 ? 'FIQ' : (cpsr & 0x1F) === 0x12 ? 'IRQ' : 'SYS';

  return (
    <div className="flex flex-col bg-[var(--color-bg-panel)] border border-[var(--color-border)] rounded-md overflow-hidden min-h-0">
      {/* Header */}
      <div className="flex items-center gap-2 px-3 py-2 bg-[var(--color-bg-card)] border-b border-[var(--color-border)] shrink-0">
        <span className="text-[var(--color-neon)] text-sm">⚙</span>
        <span className="font-[var(--font-display)] text-[11px] font-semibold text-[var(--color-txt-bright)] tracking-widest uppercase">Registers</span>
        <span className="ml-auto font-[var(--font-mono)] text-[10px] text-[var(--color-lime)] tabular-nums">CYC:{cycles}</span>
      </div>

      {/* Register list */}
      <div className="overflow-y-auto flex-1">
        {REG_NAMES.map((name, i) => (
          <div key={i}
            className={`flex items-center gap-1 px-3 py-[3px] border-b border-[var(--color-border)]/40 hover:bg-[var(--color-bg-hover)] transition-colors ${REG_ACCENT[i] ?? ''}`}>
            <span className={`w-7 text-[10px] font-semibold font-[var(--font-display)] tracking-wide ${REG_NAME_ACCENT[i] ?? 'text-[var(--color-txt-dim)]'}`}>
              {name}
            </span>
            <span className="flex-1 text-[11px] text-[var(--color-txt)] tabular-nums tracking-tight">
              0x{regs[i].toString(16).toUpperCase().padStart(8, '0')}
            </span>
            <span className="text-[9px] text-[var(--color-txt-dim)] tabular-nums">{regs[i] >>> 0}</span>
          </div>
        ))}
      </div>

      {/* CPSR section */}
      <div className="p-3 border-t border-[var(--color-border)] bg-[var(--color-bg-card)] shrink-0">
        <div className="flex items-center justify-between mb-1.5">
          <span className="text-[9px] text-[var(--color-txt-dim)] tracking-[0.12em] uppercase font-[var(--font-display)]">CPSR</span>
          <span className="text-[11px] text-[var(--color-amber)] tabular-nums">0x{cpsr.toString(16).toUpperCase().padStart(8, '0')}</span>
        </div>
        <div className="flex gap-1 mb-2">
          {FLAGS.map(({ bit, label, title }) => (
            <div key={label} title={title}
              className={`flex-1 text-center text-[10px] font-bold py-0.5 rounded-sm border transition-all cursor-default
                ${(cpsr & bit)
                  ? 'bg-[var(--color-lime)]/15 border-[var(--color-lime)]/60 text-[var(--color-lime)]'
                  : 'border-[var(--color-border-2)] text-[var(--color-txt-dim)]'
                }`}>
              {label}
            </div>
          ))}
        </div>
        <div className="flex gap-2 text-[9px] text-[var(--color-txt-dim)]">
          <span className="px-1.5 py-0.5 bg-[var(--color-bg-hover)] rounded text-[var(--color-neon)]/70 border border-[var(--color-border-2)]">{mode}</span>
          <span className="px-1.5 py-0.5 bg-[var(--color-bg-hover)] rounded border border-[var(--color-border-2)]">{(cpsr & CPSR_T) ? 'THUMB' : 'ARM'}</span>
        </div>
      </div>
    </div>
  );
}

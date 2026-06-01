import { useEmulatorStore } from '../store/emulatorStore';
import { SAMPLE_PROGRAMS } from '../emulator/cpu';

const RUN_STATE_STYLES: Record<string, string> = {
  stopped: 'text-[var(--color-txt-dim)] border-[var(--color-border-2)]',
  running: 'text-[var(--color-lime)] border-[var(--color-lime)] running-badge',
  paused:  'text-[var(--color-amber)] border-[var(--color-amber)]',
  halted:  'text-[var(--color-crimson)] border-[var(--color-crimson)]',
};

export function Toolbar() {
  const { runState, step, run, pause, stop, reset,
          selectedProgram, loadProgram,
          stepsPerSecond, setStepsPerSecond, cpuState } = useEmulatorStore();

  const isRunning = runState === 'running';
  const isHalted  = cpuState.halted;

  return (
    <header className="flex items-center gap-4 px-4 h-12 bg-[var(--color-bg-panel)] border-b border-[var(--color-border)] shrink-0 relative overflow-hidden">
      {/* Corner accent */}
      <div className="absolute top-0 left-0 w-24 h-px bg-gradient-to-r from-[var(--color-neon)] to-transparent" />
      <div className="absolute bottom-0 right-0 w-24 h-px bg-gradient-to-l from-[var(--color-neon)] to-transparent opacity-30" />

      {/* Brand */}
      <div className="flex items-baseline gap-2 mr-2">
        <span className="text-[var(--color-neon)] text-lg leading-none">◈</span>
        <span className="font-[var(--font-display)] text-base font-bold text-[var(--color-txt-bright)] tracking-widest">ARMv6</span>
        <span className="text-[9px] text-[var(--color-txt-dim)] tracking-[0.15em] uppercase">Cycle Emulator</span>
      </div>

      {/* Program selector */}
      <div className="flex gap-1">
        {Object.entries(SAMPLE_PROGRAMS).map(([key, prog]) => (
          <button
            key={key}
            onClick={() => loadProgram(key)}
            className={`px-3 py-1 text-[10px] tracking-wider border rounded-sm transition-all cursor-pointer font-[var(--font-mono)]
              ${selectedProgram === key
                ? 'border-[var(--color-neon)] text-[var(--color-neon)] bg-[var(--color-neon)]/8'
                : 'border-[var(--color-border-2)] text-[var(--color-txt-dim)] hover:border-[var(--color-neon)]/50 hover:text-[var(--color-neon)]/70'
              }`}
          >
            {prog.name}
          </button>
        ))}
      </div>

      {/* Controls */}
      <div className="flex gap-1">
        {[
          { icon: '↺', action: reset, cls: 'hover:text-[var(--color-txt)]', title: 'Reset' },
          { icon: '▷|', action: step, disabled: isRunning || isHalted, cls: 'hover:text-[var(--color-neon)]', title: 'Step' },
        ].map(({ icon, action, disabled, cls, title }) => (
          <button key={icon} onClick={action} disabled={disabled}
            title={title}
            className={`w-8 h-8 flex items-center justify-center border border-[var(--color-border-2)] bg-[var(--color-bg-card)] text-[var(--color-txt-dim)] rounded transition-all cursor-pointer text-sm
              ${cls} disabled:opacity-30 disabled:cursor-not-allowed`}>
            {icon}
          </button>
        ))}

        {isRunning ? (
          <button onClick={pause} title="Pause"
            className="w-8 h-8 flex items-center justify-center border border-[var(--color-amber)]/40 bg-[var(--color-amber)]/8 text-[var(--color-amber)] rounded transition-all cursor-pointer text-sm hover:bg-[var(--color-amber)]/15">
            ⏸
          </button>
        ) : (
          <button onClick={run} disabled={isHalted} title="Run"
            className="w-8 h-8 flex items-center justify-center border border-[var(--color-lime)]/40 bg-[var(--color-lime)]/8 text-[var(--color-lime)] rounded transition-all cursor-pointer text-sm hover:bg-[var(--color-lime)]/15 disabled:opacity-30 disabled:cursor-not-allowed">
            ▶
          </button>
        )}

        <button onClick={stop} title="Stop"
          className="w-8 h-8 flex items-center justify-center border border-[var(--color-crimson)]/40 bg-[var(--color-crimson)]/8 text-[var(--color-crimson)] rounded transition-all cursor-pointer text-sm hover:bg-[var(--color-crimson)]/15">
          ■
        </button>
      </div>

      {/* Speed */}
      <div className="flex items-center gap-2 ml-auto">
        <span className="text-[9px] text-[var(--color-txt-dim)] tracking-[0.12em] uppercase">Speed</span>
        <input type="range" min={1} max={200} value={stepsPerSecond}
          onChange={e => setStepsPerSecond(Number(e.target.value))}
          className="w-24 accent-[var(--color-neon)] h-0.5 cursor-pointer" />
        <span className="text-[11px] text-[var(--color-neon)] w-16 tabular-nums">{stepsPerSecond} IPS</span>
      </div>

      {/* Status badge */}
      <div className={`px-3 py-1 border rounded-sm text-[10px] font-semibold tracking-[0.12em] uppercase ${RUN_STATE_STYLES[runState]}`}>
        {isHalted && cpuState.exception ? `HALT` : runState}
      </div>
    </header>
  );
}

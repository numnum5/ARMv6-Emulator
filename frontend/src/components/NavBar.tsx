import { useEmulatorStore } from '../store/emulatorStore';
import type { NavTab, StepMode } from '../store/emulatorStore';
import { SAMPLE_PROGRAMS } from '../emulator/cpu';

const TABS: { id: NavTab; icon: string; label: string }[] = [
  { id: 'emulator', icon: '◈', label: 'Emulator' },
  { id: 'code',     icon: '❮❯', label: 'C Editor' },
  { id: 'upload',   icon: '↑', label: 'Upload' },
];

const RUN_BADGE: Record<string, string> = {
  stopped: 'text-[var(--color-txt-dim)] border-[var(--color-border-2)]',
  running: 'text-[var(--color-lime)] border-[var(--color-lime)] running-badge',
  paused:  'text-[var(--color-amber)] border-[var(--color-amber)]',
  halted:  'text-[var(--color-crimson)] border-[var(--color-crimson)]',
};

export function NavBar() {
  const {
    activeTab, setActiveTab,
    runState, step, run, pause, stop, reset,
    stepMode, setStepMode,
    selectedProgram, loadProgram,
    stepsPerSecond, setStepsPerSecond,
    cpuState, loadedBinary,
  } = useEmulatorStore();

  const isRunning = runState === 'running';
  const isHalted  = cpuState.halted;

  return (
    <header className="flex items-stretch h-13 bg-[var(--color-bg-panel)] border-b border-[var(--color-border)] shrink-0 relative">
      {/* Left accent line */}
      <div className="absolute top-0 left-0 right-0 h-px bg-gradient-to-r from-[var(--color-neon)] via-[var(--color-neon)]/30 to-transparent" />

      {/* Brand */}
      <div className="flex items-center gap-2 px-4 border-r border-[var(--color-border)] shrink-0">
        <span className="text-[var(--color-neon)] text-xl leading-none" style={{ textShadow: '0 0 16px var(--color-neon)' }}>◈</span>
        <div className="flex flex-col leading-tight">
          <span className="font-[var(--font-display)] text-[13px] font-bold text-[var(--color-txt-bright)] tracking-widest">ARMv6</span>
          <span className="text-[8px] text-[var(--color-txt-dim)] tracking-[0.18em] uppercase">Emulator</span>
        </div>
      </div>

      {/* Tabs */}
      <nav className="flex items-stretch border-r border-[var(--color-border)]">
        {TABS.map(({ id, icon, label }) => (
          <button key={id} onClick={() => setActiveTab(id)}
            className={`flex items-center gap-2 px-5 text-[11px] font-semibold tracking-wider uppercase border-b-2 transition-all cursor-pointer relative
              ${activeTab === id
                ? 'text-[var(--color-neon)] border-[var(--color-neon)] bg-[var(--color-neon)]/5'
                : 'text-[var(--color-txt-dim)] border-transparent hover:text-[var(--color-txt)] hover:bg-[var(--color-bg-hover)]'
              }`}>
            <span className="text-sm">{icon}</span>
            <span className="font-[var(--font-display)]">{label}</span>
          </button>
        ))}
      </nav>

      {/* Emulator controls (only when on emulator tab) */}
      {activeTab === 'emulator' && (
        <>
          {/* Sample programs */}
          <div className="flex items-center gap-1 px-3 border-r border-[var(--color-border)]">
            {Object.entries(SAMPLE_PROGRAMS).map(([key, prog]) => (
              <button key={key} onClick={() => loadProgram(key)}
                className={`px-2.5 py-1 text-[10px] tracking-wider border rounded-sm transition-all cursor-pointer font-[var(--font-display)]
                  ${selectedProgram === key && loadedBinary?.type === 'sample'
                    ? 'border-[var(--color-neon)]/60 text-[var(--color-neon)] bg-[var(--color-neon)]/8'
                    : 'border-[var(--color-border-2)] text-[var(--color-txt-dim)] hover:border-[var(--color-neon)]/40 hover:text-[var(--color-txt)]'
                  }`}>
                {prog.name}
              </button>
            ))}
          </div>

          {/* Step mode toggle */}
          <div className="flex items-center gap-1 px-3 border-r border-[var(--color-border)]">
            <span className="text-[9px] text-[var(--color-txt-dim)] uppercase tracking-wider mr-1">Step</span>
            {(['instruction', 'cycle'] as StepMode[]).map(m => (
              <button key={m} onClick={() => setStepMode(m)}
                className={`px-2.5 py-1 text-[10px] border rounded-sm transition-all cursor-pointer font-[var(--font-display)] tracking-wide
                  ${stepMode === m
                    ? 'border-[var(--color-violet)]/60 text-[var(--color-violet)] bg-[var(--color-violet)]/8'
                    : 'border-[var(--color-border-2)] text-[var(--color-txt-dim)] hover:border-[var(--color-violet)]/40'
                  }`}>
                {m === 'instruction' ? 'INSTR' : 'CYCLE'}
              </button>
            ))}
          </div>

          {/* Playback controls */}
          <div className="flex items-center gap-1 px-3 border-r border-[var(--color-border)]">
            <CtrlBtn icon="↺" onClick={reset} title="Reset" hoverColor="var(--color-txt)" />
            <CtrlBtn icon="▷|" onClick={step} disabled={isRunning || isHalted} title={stepMode === 'cycle' ? 'Step 1 Clock Cycle' : 'Step 1 Instruction'} hoverColor="var(--color-neon)" />
            {isRunning
              ? <CtrlBtn icon="⏸" onClick={pause} title="Pause" activeColor="var(--color-amber)" />
              : <CtrlBtn icon="▶" onClick={run}   disabled={isHalted} title="Run" activeColor="var(--color-lime)" />
            }
            <CtrlBtn icon="■" onClick={stop} title="Stop" activeColor="var(--color-crimson)" />
          </div>

          {/* Speed */}
          <div className="flex items-center gap-2 px-3 border-r border-[var(--color-border)]">
            <span className="text-[9px] text-[var(--color-txt-dim)] uppercase tracking-wider">IPS</span>
            <input type="range" min={1} max={200} value={stepsPerSecond}
              onChange={e => setStepsPerSecond(Number(e.target.value))}
              className="w-20 accent-[var(--color-neon)] cursor-pointer h-px" />
            <span className="text-[11px] text-[var(--color-neon)] tabular-nums w-12">{stepsPerSecond}</span>
          </div>
        </>
      )}

      {/* Right: status + cycle counter */}
      <div className="flex items-center gap-3 px-4 ml-auto">
        <span className="text-[10px] text-[var(--color-txt-dim)] tabular-nums">
          CYC <span className="text-[var(--color-lime)]">{cpuState.cycles}</span>
        </span>
        <div className={`px-2.5 py-0.5 border rounded-sm text-[9px] font-bold tracking-[0.14em] uppercase ${RUN_BADGE[runState]}`}>
          {isHalted && cpuState.exception ? 'HALT' : runState}
        </div>
      </div>
    </header>
  );
}

function CtrlBtn({ icon, onClick, disabled, title, hoverColor, activeColor }: {
  icon: string; onClick: () => void; disabled?: boolean; title: string;
  hoverColor?: string; activeColor?: string;
}) {
  const color = activeColor ?? hoverColor ?? 'var(--color-txt)';
  return (
    <button onClick={onClick} disabled={disabled} title={title}
      style={{ '--btn-color': color } as React.CSSProperties}
      className="w-8 h-8 flex items-center justify-center border border-[var(--color-border-2)] bg-[var(--color-bg-card)] text-[var(--color-txt-dim)] rounded text-sm transition-all cursor-pointer
        hover:text-[var(--btn-color)] hover:border-[var(--btn-color)]/50 hover:bg-[var(--btn-color)]/8
        disabled:opacity-25 disabled:cursor-not-allowed disabled:hover:text-[var(--color-txt-dim)] disabled:hover:border-[var(--color-border-2)] disabled:hover:bg-[var(--color-bg-card)]">
      {icon}
    </button>
  );
}

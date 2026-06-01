import { useEmulatorStore } from '../store/emulatorStore';

const STAGE_CFG = [
  { key: 'FETCH',   label: 'FETCH',   color: 'var(--color-neon)',   border: 'border-[var(--color-neon)]/50',  bg: 'bg-[var(--color-neon)]/6',  text: 'text-[var(--color-neon)]' },
  { key: 'DECODE',  label: 'DECODE',  color: 'var(--color-lime)',   border: 'border-[var(--color-lime)]/50',  bg: 'bg-[var(--color-lime)]/6',  text: 'text-[var(--color-lime)]' },
  { key: 'EXECUTE', label: 'EXECUTE', color: 'var(--color-amber)',  border: 'border-[var(--color-amber)]/50', bg: 'bg-[var(--color-amber)]/6', text: 'text-[var(--color-amber)]' },
];

export function PipelinePanel() {
  const { cpuState, cpu, stepMode, currentMicroStage } = useEmulatorStore();
  const { pipeline, cycles } = cpuState;

  const stageData = [
    { ...STAGE_CFG[0], addr: pipeline.fetchAddr,   raw: pipeline.fetch,   active: pipeline.fetch   !== null },
    { ...STAGE_CFG[1], addr: pipeline.decodeAddr,  raw: pipeline.decode,  active: pipeline.decode  !== null },
    { ...STAGE_CFG[2], addr: pipeline.executeAddr, raw: pipeline.execute, active: pipeline.execute !== null },
  ];

  const log = cpu.cycleLog.slice(-14).reverse();

  return (
    <div className="flex flex-col bg-[var(--color-bg-panel)] border border-[var(--color-border)] rounded-md overflow-hidden shrink-0">
      {/* Header */}
      <div className="flex items-center gap-2 px-3 py-2 bg-[var(--color-bg-card)] border-b border-[var(--color-border)]">
        <span className="text-[var(--color-neon)]">⟳</span>
        <span className="font-[var(--font-display)] text-[11px] font-semibold text-[var(--color-txt-bright)] tracking-widest uppercase">Pipeline</span>
        {stepMode === 'cycle' && (
          <span className="ml-1 text-[9px] px-1.5 py-0.5 border border-[var(--color-violet)]/50 text-[var(--color-violet)] rounded-sm font-[var(--font-display)] tracking-wide">
            CYCLE MODE
          </span>
        )}
        <div className="ml-auto flex items-baseline gap-2">
          <span className="text-[9px] text-[var(--color-txt-dim)]">CYCLES</span>
          <span className="font-[var(--font-display)] text-lg font-bold text-[var(--color-lime)] tabular-nums" style={{ textShadow: '0 0 10px var(--color-lime)' }}>
            {cycles}
          </span>
        </div>
      </div>

      {/* Pipeline stages */}
      <div className="flex items-center gap-2 px-3 py-3">
        {stageData.map((s, i) => {
          const isCurrentMicro = stepMode === 'cycle' && currentMicroStage === s.key;
          return (
            <div key={s.key} className="flex items-center gap-2 flex-1">
              <div className={`flex-1 border rounded p-2 text-center transition-all duration-200
                ${isCurrentMicro ? `${s.border} ${s.bg} shadow-[0_0_16px_-4px_${s.color}]` : s.active ? `${s.border} ${s.bg}` : 'border-[var(--color-border)] opacity-25'}`}
                style={isCurrentMicro ? { boxShadow: `0 0 18px -4px ${s.color}` } : undefined}>
                <div className={`font-[var(--font-display)] text-[9px] font-bold tracking-[0.15em] mb-1 ${s.active || isCurrentMicro ? s.text : 'text-[var(--color-txt-dim)]'}`}>
                  {s.label}
                  {isCurrentMicro && <span className="ml-1 animate-pulse">●</span>}
                </div>
                {s.active ? (
                  <>
                    <div className="text-[9px] text-[var(--color-txt-dim)] tabular-nums">
                      {s.addr.toString(16).toUpperCase().padStart(8, '0')}
                    </div>
                    <div className={`text-[10px] tabular-nums mt-0.5 ${s.text}`}>
                      {s.raw?.toString(16).toUpperCase().padStart(8, '0') ?? '--------'}
                    </div>
                  </>
                ) : (
                  <div className="text-[var(--color-txt-dim)] text-sm py-1">—</div>
                )}
              </div>
              {i < 2 && <span className="text-[var(--color-txt-dim)] text-sm shrink-0">→</span>}
            </div>
          );
        })}
      </div>

      {/* Execution trace */}
      <div className="border-t border-[var(--color-border)] bg-[var(--color-bg-card)] px-3 py-2">
        <div className="text-[9px] text-[var(--color-txt-dim)] uppercase tracking-[0.1em] mb-1.5">Trace</div>
        <div className="space-y-[1px] max-h-[100px] overflow-y-auto">
          {log.map((entry, i) => {
            const stageColor = entry.stage === 'FETCH' ? 'text-[var(--color-neon)]' : entry.stage === 'DECODE' ? 'text-[var(--color-lime)]' : 'text-[var(--color-txt)]';
            return (
              <div key={i} className="flex gap-2 text-[10px]" style={{ opacity: 1 - i * 0.06 }}>
                <span className="text-[var(--color-txt-dim)] tabular-nums w-10 shrink-0">#{entry.cycle}</span>
                <span className="text-[var(--color-violet)] tabular-nums w-16 shrink-0">{entry.addr.toString(16).toUpperCase().padStart(6,'0')}</span>
                <span className={`truncate ${stageColor}`}>{entry.mnemonic}</span>
                {stepMode === 'cycle' && (
                  <span className={`text-[8px] shrink-0 ${stageColor}`}>{entry.stage[0]}</span>
                )}
              </div>
            );
          })}
          {log.length === 0 && <div className="text-[var(--color-txt-dim)] text-[10px] italic">No execution yet</div>}
        </div>
      </div>
    </div>
  );
}

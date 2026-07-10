import { useEffect, useRef } from 'react';
import { useEmulatorStore } from '../store/emulatorStore';

type OutputKind = 'stdout' | 'stderr' | 'info' | 'warn' | 'error';

export interface OutputEntry {
  kind: OutputKind;
  text: string;
  cycle?: number;
}

const KIND_STYLE: Record<OutputKind, { dot: string; text: string; prefix: string }> = {
  stdout: { dot: 'bg-[var(--color-lime)]',    text: 'text-[var(--color-txt)]',        prefix: '' },
  stderr: { dot: 'bg-[var(--color-crimson)]', text: 'text-[var(--color-crimson)]/90', prefix: 'ERR ' },
  info:   { dot: 'bg-[var(--color-neon)]',    text: 'text-[var(--color-neon)]/80',    prefix: 'INF ' },
  warn:   { dot: 'bg-[var(--color-amber)]',   text: 'text-[var(--color-amber)]/90',   prefix: 'WRN ' },
  error:  { dot: 'bg-[var(--color-crimson)]', text: 'text-[var(--color-crimson)]',    prefix: 'ERR ' },
};

// Stable empty fallback — defined outside the component so its reference
// never changes. Putting `?? []` inside a selector creates a new array on
// every call, which breaks Zustand's snapshot cache and causes an infinite loop.
const EMPTY_LOG: OutputEntry[] = [];

export function OutputPanel() {
  // Keep selectors free of fallback expressions so Zustand can do a stable
  // reference comparison between snapshots.
  const rawLog        = useEmulatorStore((s: any) => s.outputLog  as OutputEntry[] | undefined);
  const clearOutputLog = useEmulatorStore((s: any) => s.clearOutputLog as (() => void) | undefined);


  console.log(rawLog)

  const outputLog = rawLog ?? EMPTY_LOG;

  const bottomRef = useRef<HTMLDivElement>(null);

  useEffect(() => {
    bottomRef.current?.scrollIntoView({ behavior: 'smooth' });
  }, [outputLog.length]);

  return (
    <div className="flex flex-col bg-[var(--color-bg-panel)] border border-[var(--color-border)] rounded-md overflow-hidden min-h-0 flex-1">
      {/* Header */}
      <div className="flex items-center gap-2 px-3 py-2 bg-[var(--color-bg-card)] border-b border-[var(--color-border)] shrink-0">
        <span className="text-[var(--color-lime)] text-sm leading-none">▸</span>
        <span className="font-[var(--font-display)] text-[11px] font-semibold text-[var(--color-txt-bright)] tracking-widest uppercase">Output</span>

        {/* Legend */}
        <div className="flex items-center gap-3 ml-3 text-[8px] text-[var(--color-txt-dim)]">
          <span className="flex items-center gap-1"><span className="inline-block w-1.5 h-1.5 rounded-full bg-[var(--color-lime)]" />stdout</span>
          <span className="flex items-center gap-1"><span className="inline-block w-1.5 h-1.5 rounded-full bg-[var(--color-crimson)]" />stderr</span>
          <span className="flex items-center gap-1"><span className="inline-block w-1.5 h-1.5 rounded-full bg-[var(--color-neon)]" />info</span>
          <span className="flex items-center gap-1"><span className="inline-block w-1.5 h-1.5 rounded-full bg-[var(--color-amber)]" />warn</span>
        </div>

        {/* Line count + clear */}
        <div className="ml-auto flex items-center gap-2">
          <span className="text-[9px] text-[var(--color-txt-dim)] tabular-nums">
            {outputLog.length} line{outputLog.length !== 1 ? 's' : ''}
          </span>
          {clearOutputLog && (
            <button
              onClick={clearOutputLog}
              title="Clear output"
              className="px-2 py-0.5 text-[9px] font-[var(--font-display)] tracking-wider border border-[var(--color-border-2)] text-[var(--color-txt-dim)] rounded-sm hover:border-[var(--color-crimson)]/50 hover:text-[var(--color-crimson)] transition-colors cursor-pointer">
              CLEAR
            </button>
          )}
        </div>
      </div>

      {/* Log body */}
      <div className="overflow-y-auto flex-1 font-[var(--font-mono)] text-[11px] leading-relaxed">
        {outputLog.length === 0 ? (
          <div className="flex flex-col items-center justify-center h-full gap-2 text-[var(--color-txt-dim)] select-none">
            <span className="text-2xl opacity-20">▸</span>
            <span className="text-[10px] tracking-widest uppercase">No output yet</span>
            <span className="text-[9px] opacity-60">SWI write calls will appear here</span>
          </div>
        ) : (
          <div className="p-2 space-y-[1px]">
            {outputLog.map((entry, i) => {
              const style = KIND_STYLE[entry.kind] ?? KIND_STYLE.info;
              return (
                <div key={i}
                  className="flex items-start gap-2 px-2 py-[2px] rounded-sm hover:bg-[var(--color-bg-hover)] transition-colors group">

                  {/* Kind dot */}
                  <span className={`mt-[5px] w-1.5 h-1.5 rounded-full shrink-0 ${style.dot}`} />

                  {/* Cycle number */}
                  {entry.cycle !== undefined && (
                    <span className="text-[9px] text-[var(--color-txt-dim)] tabular-nums w-10 shrink-0 pt-px">
                      #{entry.cycle}
                    </span>
                  )}

                  {/* Kind prefix (for non-stdout) */}
                  {style.prefix && (
                    <span className={`text-[9px] shrink-0 pt-px opacity-60 ${style.text}`}>
                      {style.prefix}
                    </span>
                  )}

                  {/* Text — preserve whitespace, wrap */}
                  <span className={`whitespace-pre-wrap break-all ${style.text}`}>
                    {entry.text}
                  </span>
                </div>
              );
            })}
            <div ref={bottomRef} />
          </div>
        )}
      </div>

      {/* Footer: last stdout line preview */}
      {outputLog.length > 0 && (
        <div className="shrink-0 border-t border-[var(--color-border)] bg-[var(--color-bg-base)] px-3 py-1.5">
          <div className="flex items-center gap-2 text-[9px] text-[var(--color-txt-dim)]">
            <span className="text-[var(--color-lime)]/60">$</span>
            <span className="truncate text-[var(--color-txt-dim)]/60 italic">
              {outputLog.filter(e => e.kind === 'stdout').slice(-1)[0]?.text.trim().slice(0, 80) ?? '—'}
            </span>
          </div>
        </div>
      )}
    </div>
  );
}
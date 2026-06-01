import { useState } from 'react';
import { useEmulatorStore } from '../store/emulatorStore';

const COLS = 16;
const ROWS = 14;

export function MemoryPanel() {
  const { cpu, cpuState, memViewAddr, setMemViewAddr } = useEmulatorStore();
  const [inputVal, setInputVal] = useState('');
  const base = memViewAddr & ~0xF;
  const pc = cpuState.regs[15];
  const sp = cpuState.regs[13];

  const handleJump = () => {
    const addr = parseInt(inputVal, 16);
    if (!isNaN(addr)) setMemViewAddr(addr & ~0xF);
  };

  const rows = Array.from({ length: ROWS }, (_, r) => {
    const rowAddr = base + r * COLS;
    const bytes = Array.from({ length: COLS }, (_, c) => cpu.read8(rowAddr + c));
    return { rowAddr, bytes };
  });

  const NavBtn = ({ onClick, label }: { onClick: () => void; label: string }) => (
    <button onClick={onClick}
      className="px-2 py-0.5 text-[10px] bg-[var(--color-bg-hover)] border border-[var(--color-border-2)] text-[var(--color-txt-dim)] rounded-sm hover:text-[var(--color-neon)] hover:border-[var(--color-neon)]/50 cursor-pointer transition-colors">
      {label}
    </button>
  );

  return (
    <div className="flex flex-col bg-[var(--color-bg-panel)] border border-[var(--color-border)] rounded-md overflow-hidden flex-1 min-h-0">
      {/* Header */}
      <div className="flex items-center gap-2 px-3 py-2 bg-[var(--color-bg-card)] border-b border-[var(--color-border)] shrink-0">
        <span className="text-[var(--color-neon)]">▤</span>
        <span className="font-[var(--font-display)] text-[11px] font-semibold text-[var(--color-txt-bright)] tracking-widest uppercase">Memory</span>
        <div className="ml-auto flex items-center gap-1">
          <input
            value={inputVal}
            onChange={e => setInputVal(e.target.value)}
            onKeyDown={e => e.key === 'Enter' && handleJump()}
            placeholder="hex addr"
            className="w-20 px-2 py-0.5 text-[10px] bg-[var(--color-bg-base)] border border-[var(--color-border-2)] text-[var(--color-txt)] rounded-sm outline-none focus:border-[var(--color-neon)]/60 placeholder:text-[var(--color-txt-dim)] font-[var(--font-mono)]"
          />
          <NavBtn onClick={handleJump} label="Go" />
          <NavBtn onClick={() => setMemViewAddr(pc & ~0xF)} label="PC" />
          <NavBtn onClick={() => setMemViewAddr(sp & ~0xF)} label="SP" />
        </div>
      </div>

      {/* Legend */}
      <div className="flex gap-4 px-3 py-1 text-[9px] border-b border-[var(--color-border)] bg-[var(--color-bg-card)]/50 shrink-0">
        <span><span className="inline-block w-3 h-2 bg-[var(--color-neon)]/20 border border-[var(--color-neon)]/40 rounded-sm mr-1" />PC</span>
        <span><span className="inline-block w-3 h-2 bg-[var(--color-amber)]/20 border border-[var(--color-amber)]/40 rounded-sm mr-1" />SP</span>
        <span className="text-[var(--color-txt-dim)]">click to scroll</span>
      </div>

      {/* Hex dump */}
      <div className="overflow-auto flex-1 text-[10.5px] font-[var(--font-mono)]">
        <table className="w-full border-collapse">
          <thead className="sticky top-0 bg-[var(--color-bg-card)] z-10">
            <tr>
              <th className="text-left px-3 py-1 text-[var(--color-txt-dim)] font-normal text-[9px] tracking-wider uppercase whitespace-nowrap border-b border-[var(--color-border)]">
                Address
              </th>
              {Array.from({ length: COLS }, (_, i) => (
                <th key={i} className="px-1 py-1 text-center text-[var(--color-txt-dim)] font-normal text-[9px] border-b border-[var(--color-border)] w-6">
                  {i.toString(16).toUpperCase()}
                </th>
              ))}
              <th className="px-3 py-1 text-left text-[var(--color-txt-dim)] font-normal text-[9px] tracking-wider uppercase border-b border-[var(--color-border)]">
                ASCII
              </th>
            </tr>
          </thead>
          <tbody>
            {rows.map(({ rowAddr, bytes }) => (
              <tr key={rowAddr} className="hover:bg-[var(--color-bg-hover)] transition-colors">
                <td className="px-3 py-[2px] text-[var(--color-txt-dim)] whitespace-nowrap">
                  {rowAddr.toString(16).toUpperCase().padStart(8, '0')}
                </td>
                {bytes.map((b, c) => {
                  const byteAddr = rowAddr + c;
                  const isPC = byteAddr >= (pc & ~3) && byteAddr < (pc & ~3) + 4;
                  const isSP = byteAddr >= (sp & ~3) && byteAddr < (sp & ~3) + 4;
                  return (
                    <td key={c}
                      className={`px-1 py-[2px] text-center w-6 tabular-nums
                        ${isPC ? 'bg-[var(--color-neon)]/15 text-[var(--color-neon)]'
                               : isSP ? 'bg-[var(--color-amber)]/12 text-[var(--color-amber)]'
                               : b === 0 ? 'text-[var(--color-border-2)]'
                               : 'text-[var(--color-txt)]'}`}>
                      {b.toString(16).toUpperCase().padStart(2, '0')}
                    </td>
                  );
                })}
                <td className="px-3 py-[2px] text-[var(--color-txt-dim)] whitespace-nowrap tracking-tight">
                  {bytes.map((b, c) => (
                    <span key={c} className={b === 0 ? 'text-[var(--color-border-2)]' : b > 31 && b < 127 ? 'text-[var(--color-txt)]' : ''}>
                      {b >= 32 && b < 127 ? String.fromCharCode(b) : '.'}
                    </span>
                  ))}
                </td>
              </tr>
            ))}
          </tbody>
        </table>
      </div>
    </div>
  );
}

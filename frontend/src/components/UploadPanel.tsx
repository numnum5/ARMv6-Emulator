import { useRef, useState } from 'react';
import { useEmulatorStore } from '../store/emulatorStore';

export function UploadPanel() {
  const { loadELFFile, loadRawBinary, loadedBinary, setActiveTab } = useEmulatorStore();
  const [dragging, setDragging] = useState(false);
  const [loading, setLoading] = useState(false);
  const [rawEntry, setRawEntry] = useState('0x00000000');
  const elfRef = useRef<HTMLInputElement>(null);
  const rawRef = useRef<HTMLInputElement>(null);

  async function handleFile(file: File, isRaw = false) {
    setLoading(true);
    try {
      if (isRaw) {
        const entry = parseInt(rawEntry, 16) || 0;
        await loadRawBinary(file, entry);
      } else {
        await loadELFFile(file);
      }
      setActiveTab('emulator');
    } finally {
      setLoading(false);
    }
  }

  function onDrop(e: React.DragEvent) {
    e.preventDefault();
    setDragging(false);
    const file = e.dataTransfer.files[0];
    if (!file) return;
    const isRaw = file.name.endsWith('.bin') || file.name.endsWith('.raw');
    handleFile(file, isRaw);
  }

  return (
    <div className="flex-1 overflow-y-auto p-6 bg-[var(--color-bg-base)]">
      <div className="max-w-2xl mx-auto space-y-6">

        {/* Header */}
        <div>
          <h2 className="font-[var(--font-display)] text-2xl font-bold text-[var(--color-txt-bright)] tracking-widest mb-1">Upload Binary</h2>
          <p className="text-[var(--color-txt-dim)] text-sm">Load an ARMv6 ELF binary or raw binary into the emulator</p>
        </div>

        {/* Current loaded binary */}
        {loadedBinary && (
          <div className={`p-4 rounded border ${loadedBinary.error
            ? 'border-[var(--color-crimson)]/40 bg-[var(--color-crimson)]/5'
            : 'border-[var(--color-lime)]/40 bg-[var(--color-lime)]/5'}`}>
            <div className="flex items-center gap-2 mb-2">
              <span className={loadedBinary.error ? 'text-[var(--color-crimson)]' : 'text-[var(--color-lime)]'}>
                {loadedBinary.error ? '✗' : '✓'}
              </span>
              <span className="font-[var(--font-display)] text-sm font-semibold text-[var(--color-txt-bright)] tracking-wide">
                {loadedBinary.name}
              </span>
              <span className="ml-auto text-[10px] text-[var(--color-txt-dim)] uppercase tracking-wider px-2 py-0.5 border border-[var(--color-border-2)] rounded">
                {loadedBinary.type}
              </span>
            </div>
            {loadedBinary.error ? (
              <p className="text-[var(--color-crimson)] text-xs">{loadedBinary.error}</p>
            ) : (
              <div className="grid grid-cols-2 gap-2 text-xs">
                <div><span className="text-[var(--color-txt-dim)]">Entry Point: </span>
                  <span className="text-[var(--color-neon)] tabular-nums font-mono">0x{loadedBinary.entryPoint.toString(16).toUpperCase().padStart(8,'0')}</span></div>
                <div><span className="text-[var(--color-txt-dim)]">Size: </span>
                  <span className="text-[var(--color-txt)] tabular-nums">{(loadedBinary.size / 1024).toFixed(1)} KB</span></div>
              </div>
            )}
          </div>
        )}

        {/* ELF drop zone */}
        <div
          onDragOver={e => { e.preventDefault(); setDragging(true); }}
          onDragLeave={() => setDragging(false)}
          onDrop={onDrop}
          onClick={() => elfRef.current?.click()}
          className={`relative border-2 border-dashed rounded-lg p-10 text-center cursor-pointer transition-all
            ${dragging
              ? 'border-[var(--color-neon)] bg-[var(--color-neon)]/8 scale-[1.01]'
              : 'border-[var(--color-border-2)] hover:border-[var(--color-neon)]/50 hover:bg-[var(--color-bg-card)]'
            }`}>
          <input ref={elfRef} type="file" accept=".elf,.axf" className="hidden"
            onChange={e => { const f = e.target.files?.[0]; if (f) handleFile(f, false); }} />
          <div className="text-4xl mb-3 text-[var(--color-neon)]">⬡</div>
          <p className="font-[var(--font-display)] text-sm font-semibold text-[var(--color-txt-bright)] tracking-wide mb-1">
            Drop ELF Binary
          </p>
          <p className="text-[var(--color-txt-dim)] text-xs">
            .elf / .axf — compiled ARM ELF32 binary<br />
            <span className="text-[var(--color-neon)]/60">arm-none-eabi-gcc -march=armv6 -nostdlib -o out.elf main.c</span>
          </p>
          {loading && (
            <div className="absolute inset-0 flex items-center justify-center bg-[var(--color-bg-base)]/80 rounded-lg">
              <span className="text-[var(--color-neon)] font-[var(--font-display)] text-sm tracking-widest animate-pulse">PARSING…</span>
            </div>
          )}
        </div>

        {/* Raw binary */}
        <div className="border border-[var(--color-border)] rounded-lg p-4 bg-[var(--color-bg-panel)]">
          <h3 className="font-[var(--font-display)] text-sm font-semibold text-[var(--color-txt-bright)] tracking-wide mb-3">Raw Binary (.bin)</h3>
          <div className="flex items-center gap-3">
            <div className="flex items-center gap-2 flex-1">
              <span className="text-[10px] text-[var(--color-txt-dim)] uppercase tracking-wider whitespace-nowrap">Entry Addr</span>
              <input
                value={rawEntry}
                onChange={e => setRawEntry(e.target.value)}
                className="w-32 px-2 py-1 bg-[var(--color-bg-base)] border border-[var(--color-border-2)] text-[var(--color-txt)] text-xs font-mono rounded outline-none focus:border-[var(--color-neon)]/60"
              />
            </div>
            <button
              onClick={() => rawRef.current?.click()}
              className="px-4 py-1.5 bg-[var(--color-bg-card)] border border-[var(--color-border-2)] text-[var(--color-txt-dim)] text-[11px] font-[var(--font-display)] tracking-wider rounded hover:border-[var(--color-amber)]/60 hover:text-[var(--color-amber)] transition-all cursor-pointer">
              Choose .bin File
            </button>
            <input ref={rawRef} type="file" accept=".bin,.raw" className="hidden"
              onChange={e => { const f = e.target.files?.[0]; if (f) handleFile(f, true); }} />
          </div>
        </div>

        {/* Instructions */}
        <div className="border border-[var(--color-border)] rounded-lg p-4 bg-[var(--color-bg-panel)] space-y-2">
          <h3 className="font-[var(--font-display)] text-sm font-semibold text-[var(--color-txt-bright)] tracking-wide mb-3">Compile Commands</h3>
          {[
            { label: 'GCC (bare metal)', cmd: 'arm-none-eabi-gcc -march=armv6 -O1 -nostdlib -Ttext=0x0 -o out.elf main.c' },
            { label: 'Clang', cmd: 'clang --target=arm-none-eabi -march=armv6 -nostdlib -Wl,-Ttext=0x0 -o out.elf main.c' },
            { label: 'Strip debug (optional)', cmd: 'arm-none-eabi-strip -s out.elf' },
          ].map(({ label, cmd }) => (
            <div key={label}>
              <span className="text-[9px] text-[var(--color-txt-dim)] uppercase tracking-wider">{label}</span>
              <div className="mt-1 px-3 py-2 bg-[var(--color-bg-base)] border border-[var(--color-border)] rounded font-mono text-[11px] text-[var(--color-neon)]/80 select-all">
                {cmd}
              </div>
            </div>
          ))}
        </div>

      </div>
    </div>
  );
}

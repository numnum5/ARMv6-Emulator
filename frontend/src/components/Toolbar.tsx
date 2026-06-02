import { useEmulatorStore } from '../store/emulatorStore';
import { SAMPLE_PROGRAMS } from '../emulator/cpu';
import { useState, useEffect, useRef, useCallback } from 'react';

const RUN_STATE_STYLES: Record<string, string> = {
  stopped: 'text-[var(--color-txt-dim)] border-[var(--color-border-2)]',
  running: 'text-[var(--color-lime)] border-[var(--color-lime)] running-badge',
  paused:  'text-[var(--color-amber)] border-[var(--color-amber)]',
  halted:  'text-[var(--color-crimson)] border-[var(--color-crimson)]',
};

type WsStatus = 'disconnected' | 'connecting' | 'connected' | 'error';

const WS_STATUS_STYLES: Record<WsStatus, string> = {
  disconnected: 'border-[var(--color-border-2)] text-[var(--color-txt-dim)]',
  connecting:   'border-[var(--color-amber)]/60 text-[var(--color-amber)]',
  connected:    'border-[var(--color-lime)]/60 text-[var(--color-lime)]',
  error:        'border-[var(--color-crimson)]/60 text-[var(--color-crimson)]',
};

function mapBackendState(raw: any) {
  return {
    regs: new Uint32Array(raw.regs),
    cpsr: raw.cpsr,
    spsr: raw.spsr,
    cycles: raw.cycles,
    halted: raw.halted,
    exception: raw.exception || null,
    pipeline: {
      fetch:       raw.fetch       ?? null,
      decode:      raw.decode      ?? null,
      execute:     raw.execute     ?? null,
      fetchAddr:   raw.fetchAddr   ?? 0,
      decodeAddr:  raw.decodeAddr  ?? 0,
      executeAddr: raw.executeAddr ?? 0,
    },
  };
}

export function Toolbar() {
  const { runState, step, run, pause, stop, reset,
          selectedProgram, loadProgram,
          stepsPerSecond, setStepsPerSecond, cpuState } = useEmulatorStore();

  const [wsUrl, setWsUrl]       = useState('ws://localhost:8080');
  const [wsStatus, setWsStatus] = useState<WsStatus>('disconnected');
  const [showUrlInput, setShowUrlInput] = useState(false);
  const wsRef = useRef<WebSocket | null>(null);

  const isRunning  = runState === 'running';
  const isHalted   = cpuState.halted;
  const isBackend  = wsStatus === 'connected';

  // Incoming messages → push into store
  const handleMessage = useCallback((event: MessageEvent) => {
    try {


      console.log("message received\n");
      const resp = JSON.parse(event.data);


      if (resp.type === 'state') {
        useEmulatorStore.setState({
          cpuState: mapBackendState(resp.state),
          runState: resp.state.halted ? 'halted' : 'paused',
        });
      }
    } catch {
      console.warn('WS: bad JSON', event.data);
    }
  }, []);

  const connect = useCallback(() => {
    wsRef.current?.close();
    setWsStatus('connecting');
    const ws = new WebSocket(wsUrl);
    wsRef.current = ws;
    ws.onopen    = () => setWsStatus('connected');
    ws.onclose   = () => setWsStatus('disconnected');
    ws.onerror   = () => setWsStatus('error');
    ws.onmessage = handleMessage;
  }, [wsUrl, handleMessage]);

  const disconnect = useCallback(() => {
    wsRef.current?.close();
    wsRef.current = null;
    setWsStatus('disconnected');
  }, []);

  // Clean up on unmount
  useEffect(() => () => wsRef.current?.close(), []);

  const wsSend = (msg: string) => wsRef.current?.readyState === WebSocket.OPEN
    && wsRef.current.send(msg);

  // Action routing — ws when connected, local otherwise
  const handleStep  = isBackend ? () => wsSend('step')  : step;
  const handleRun   = isBackend ? () => wsSend('run')   : run;
  const handlePause = isBackend ? () => wsSend('pause') : pause;
  const handleStop  = isBackend ? () => wsSend('stop')  : stop;
  const handleReset = isBackend ? () => wsSend('reset') : reset;

  return (
    <header className="flex items-center gap-4 px-4 h-12 bg-[var(--color-bg-panel)] border-b border-[var(--color-border)] shrink-0 relative overflow-hidden">
      <div className="absolute top-0 left-0 w-24 h-px bg-gradient-to-r from-[var(--color-neon)] to-transparent" />
      <div className="absolute bottom-0 right-0 w-24 h-px bg-gradient-to-l from-[var(--color-neon)] to-transparent opacity-30" />

      {/* Brand */}
      <div className="flex items-baseline gap-2 mr-2">
        <span className="text-[var(--color-neon)] text-lg leading-none">◈</span>
        <span className="font-[var(--font-display)] text-base font-bold text-[var(--color-txt-bright)] tracking-widest">ARMv6</span>
        <span className="text-[9px] text-[var(--color-txt-dim)] tracking-[0.15em] uppercase">Cycle Emulator</span>
      </div>

      {/* Program selector — only shown when not using backend */}
      {!isBackend && (
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
      )}

      {/* Controls */}
      <div className="flex gap-1">
        {[
          { icon: '↺', action: handleReset, cls: 'hover:text-[var(--color-txt)]', title: 'Reset' },
          { icon: '▷|', action: handleStep, disabled: isRunning || isHalted, cls: 'hover:text-[var(--color-neon)]', title: 'Step' },
        ].map(({ icon, action, disabled, cls, title }) => (
          <button key={icon} onClick={action} disabled={disabled}
            title={title}
            className={`w-8 h-8 flex items-center justify-center border border-[var(--color-border-2)] bg-[var(--color-bg-card)] text-[var(--color-txt-dim)] rounded transition-all cursor-pointer text-sm
              ${cls} disabled:opacity-30 disabled:cursor-not-allowed`}>
            {icon}
          </button>
        ))}

        {isRunning ? (
          <button onClick={handlePause} title="Pause"
            className="w-8 h-8 flex items-center justify-center border border-[var(--color-amber)]/40 bg-[var(--color-amber)]/8 text-[var(--color-amber)] rounded transition-all cursor-pointer text-sm hover:bg-[var(--color-amber)]/15">
            ⏸
          </button>
        ) : (
          <button onClick={handleRun} disabled={isHalted} title="Run"
            className="w-8 h-8 flex items-center justify-center border border-[var(--color-lime)]/40 bg-[var(--color-lime)]/8 text-[var(--color-lime)] rounded transition-all cursor-pointer text-sm hover:bg-[var(--color-lime)]/15 disabled:opacity-30 disabled:cursor-not-allowed">
            ▶
          </button>
        )}

        <button onClick={handleStop} title="Stop"
          className="w-8 h-8 flex items-center justify-center border border-[var(--color-crimson)]/40 bg-[var(--color-crimson)]/8 text-[var(--color-crimson)] rounded transition-all cursor-pointer text-sm hover:bg-[var(--color-crimson)]/15">
          ■
        </button>
      </div>

      {/* Speed — hidden in backend mode (server controls timing) */}
      {!isBackend && (
        <div className="flex items-center gap-2 ml-auto">
          <span className="text-[9px] text-[var(--color-txt-dim)] tracking-[0.12em] uppercase">Speed</span>
          <input type="range" min={1} max={200} value={stepsPerSecond}
            onChange={e => setStepsPerSecond(Number(e.target.value))}
            className="w-24 accent-[var(--color-neon)] h-0.5 cursor-pointer" />
          <span className="text-[11px] text-[var(--color-neon)] w-16 tabular-nums">{stepsPerSecond} IPS</span>
        </div>
      )}

      {/* WS connect controls */}
      <div className={`flex items-center gap-2 ${isBackend ? 'ml-auto' : ''}`}>
        {showUrlInput && (
          <input
            type="text"
            value={wsUrl}
            onChange={e => setWsUrl(e.target.value)}
            onKeyDown={e => { if (e.key === 'Enter') { connect(); setShowUrlInput(false); } }}
            className="h-7 px-2 text-[11px] font-[var(--font-mono)] bg-[var(--color-bg-card)] border border-[var(--color-border-2)] rounded text-[var(--color-txt)] w-48 focus:outline-none focus:border-[var(--color-neon)]/60"
          />
        )}
        <button
          onClick={() => setShowUrlInput(v => !v)}
          title="Configure backend URL"
          className="h-7 px-2 text-[10px] border border-[var(--color-border-2)] text-[var(--color-txt-dim)] rounded hover:text-[var(--color-txt)] transition-all cursor-pointer"
        >
          ⚙
        </button>
        {wsStatus === 'connected' ? (
          <button onClick={disconnect}
            className="h-7 px-3 text-[10px] tracking-wider border border-[var(--color-crimson)]/50 text-[var(--color-crimson)] rounded hover:bg-[var(--color-crimson)]/10 transition-all cursor-pointer">
            DISCONNECT
          </button>
        ) : (
          <button onClick={connect} disabled={wsStatus === 'connecting'}
            className="h-7 px-3 text-[10px] tracking-wider border border-[var(--color-neon)]/50 text-[var(--color-neon)] rounded hover:bg-[var(--color-neon)]/8 transition-all cursor-pointer disabled:opacity-40 disabled:cursor-not-allowed">
            {wsStatus === 'connecting' ? 'CONNECTING…' : 'CONNECT'}
          </button>
        )}
        <div className={`px-2 py-0.5 border rounded-sm text-[9px] font-semibold tracking-[0.12em] uppercase ${WS_STATUS_STYLES[wsStatus]}`}>
          {wsStatus === 'connected' ? '⬤ BACKEND' : wsStatus === 'connecting' ? '◌ …' : '○ LOCAL'}
        </div>
      </div>

      {/* Run state badge */}
      <div className={`px-3 py-1 border rounded-sm text-[10px] font-semibold tracking-[0.12em] uppercase ${RUN_STATE_STYLES[runState]}`}>
        {isHalted && cpuState.exception ? 'HALT' : runState}
      </div>
    </header>
  );
}
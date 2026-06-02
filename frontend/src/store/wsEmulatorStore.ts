import { useEmulatorStore } from './emulatorStore';

let socket: WebSocket | null = null;

export function connectToBackend(url = 'ws://localhost:8080') {
  socket = new WebSocket(url);

  socket.onmessage = (event) => {
    const resp = JSON.parse(event.data);
    
    if (resp.type === 'state') {
      const raw = resp.state;
      // Map backend response into the CPUState shape
      useEmulatorStore.setState({
        cpuState: {
          regs: new Uint32Array(raw.regs),
          cpsr: raw.cpsr,
          spsr: raw.spsr,
          cycles: raw.cycles,
          halted: raw.halted,
          exception: raw.exception || null,
          pipeline: {
            fetch: raw.fetch ?? null,
            decode: raw.decode ?? null,
            execute: raw.execute ?? null,
            fetchAddr: raw.fetchAddr,
            decodeAddr: raw.decodeAddr,
            executeAddr: raw.executeAddr,
          },
        },
        runState: raw.halted ? 'halted' : 'paused',
      });
    }
  };

  socket.onclose = () => { socket = null; };
}

// Drop-in replacements for store actions
export function wsStep() {
  socket?.send('step');
}

export function wsStepCycle() {
  socket?.send('cycle');
}

export function wsRun() {
  socket?.send('run');
}

export function wsPause() {
  socket?.send('pause');
}
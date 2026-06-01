import { NavBar } from './components/NavBar';
import { RegisterPanel } from './components/RegisterPanel';
import { DisassemblerPanel } from './components/DisassemblerPanel';
import { MemoryPanel } from './components/MemoryPanel';
import { PipelinePanel } from './components/PipelinePanel';
import { UploadPanel } from './components/UploadPanel';
import { CodeEditor } from './components/CodeEditor';
import { useEmulatorStore } from './store/emulatorStore';

function App() {
  const { activeTab } = useEmulatorStore();

  return (
    <div className="flex flex-col h-screen bg-[var(--color-bg-base)] overflow-hidden">
      <NavBar />

      {activeTab === 'emulator' && (
        <div className="grid grid-cols-[220px_1fr_340px] gap-1.5 p-1.5 flex-1 overflow-hidden min-h-0">
          <RegisterPanel />
          <DisassemblerPanel />
          <div className="flex flex-col gap-1.5 min-h-0 overflow-hidden">
            <PipelinePanel />
            <MemoryPanel />
          </div>
        </div>
      )}

      {activeTab === 'code' && (
        <div className="flex flex-1 overflow-hidden min-h-0">
          <CodeEditor />
        </div>
      )}

      {activeTab === 'upload' && (
        <div className="flex flex-1 overflow-hidden min-h-0">
          <UploadPanel />
        </div>
      )}
    </div>
  );
}

export default App;

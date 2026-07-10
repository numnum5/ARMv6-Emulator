import { NavBar } from './components/NavBar';
import { RegisterPanel } from './components/RegisterPanel';
import { DisassemblerPanel } from './components/DisassemblerPanel';
import { MemoryPanel } from './components/MemoryPanel';
import { PipelinePanel } from './components/PipelinePanel';
import { UploadPanel } from './components/UploadPanel';
import { CodeEditor } from './components/CodeEditor';
import { OutputPanel } from './components/OutputPanel';
import { useEmulatorStore } from './store/emulatorStore';

function App() {
  const { activeTab } = useEmulatorStore();
  
  return (
    <div className="flex flex-col h-screen bg-[var(--color-bg-base)] overflow-hidden">
      <NavBar />

      {activeTab === 'emulator' && (
        <div className="grid grid-cols-[220px_1fr_380px] gap-1.5 p-1.5 flex-1 overflow-hidden min-h-0">

          <RegisterPanel />

          <div className="flex flex-col gap-1.5 min-h-0 overflow-hidden">
            <div className="shrink-0 max-h-[240px] overflow-hidden flex flex-col">
              <DisassemblerPanel />
            </div>
            <MemoryPanel />
          </div>

          <div className="flex flex-col gap-1.5 min-h-0 overflow-hidden">
            <PipelinePanel />
            <OutputPanel />
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
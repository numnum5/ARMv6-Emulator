import Editor from '@monaco-editor/react';
import { useEmulatorStore } from '../store/emulatorStore';

const MONACO_THEME: object = {
  base: 'vs-dark',
  inherit: true,
  rules: [
    { token: '',            foreground: 'B8CCE0', background: '060810' },
    { token: 'comment',     foreground: '3D5A78', fontStyle: 'italic' },
    { token: 'keyword',     foreground: '00F5FF' },
    { token: 'type',        foreground: '9B6DFF' },
    { token: 'number',      foreground: 'FFB020' },
    { token: 'string',      foreground: '39FF7A' },
    { token: 'identifier',  foreground: 'B8CCE0' },
    { token: 'delimiter',   foreground: '3D4E68' },
    { token: 'operator',    foreground: '4488FF' },
  ],
  colors: {
    'editor.background':           '#060810',
    'editor.foreground':           '#B8CCE0',
    'editor.lineHighlightBackground': '#0F1420',
    'editor.selectionBackground':  '#00F5FF22',
    'editor.inactiveSelectionBackground': '#00F5FF11',
    'editorCursor.foreground':     '#00F5FF',
    'editorLineNumber.foreground': '#3D4E68',
    'editorLineNumber.activeForeground': '#00F5FF88',
    'editorGutter.background':     '#0B0E18',
    'editorIndentGuide.background1': '#1C2438',
    'editorWidget.background':     '#0B0E18',
    'editorWidget.border':         '#1C2438',
    'input.background':            '#060810',
    'input.foreground':            '#B8CCE0',
    'input.border':                '#1C2438',
    'scrollbar.shadow':            '#00000000',
    'scrollbarSlider.background':  '#252E4560',
    'scrollbarSlider.hoverBackground': '#3D4E6880',
  },
};

export function CodeEditor() {
  const { cCode, setCCode, setActiveTab } = useEmulatorStore();

  function handleMount(_editor: unknown, monaco: unknown) {
    // eslint-disable-next-line @typescript-eslint/no-explicit-any
    (monaco as any).editor.defineTheme('armv6', MONACO_THEME);
    // eslint-disable-next-line @typescript-eslint/no-explicit-any
    (monaco as any).editor.setTheme('armv6');
  }

  return (
    <div className="flex flex-col flex-1 overflow-hidden bg-[var(--color-bg-base)]">
      {/* Sub-toolbar */}
      <div className="flex items-center gap-3 px-4 py-2 bg-[var(--color-bg-panel)] border-b border-[var(--color-border)] shrink-0">
        <span className="font-[var(--font-display)] text-[11px] text-[var(--color-txt-dim)] tracking-widest uppercase">C Source</span>
        <span className="text-[10px] text-[var(--color-txt-dim)] ml-2">main.c</span>
        <div className="ml-auto flex items-center gap-2">
          <span className="text-[10px] text-[var(--color-amber)] border border-[var(--color-amber)]/40 px-2 py-0.5 rounded text-[9px]">
            Read-only preview — compile locally, then upload ELF
          </span>
          <button
            onClick={() => setActiveTab('upload')}
            className="px-3 py-1 text-[10px] font-[var(--font-display)] tracking-wider border border-[var(--color-neon)]/50 text-[var(--color-neon)] bg-[var(--color-neon)]/8 rounded hover:bg-[var(--color-neon)]/15 transition-all cursor-pointer">
            Upload ELF →
          </button>
        </div>
      </div>

      {/* Monaco Editor */}
      <div className="flex-1 min-h-0">
        <Editor
          language="c"
          value={cCode}
          onChange={v => setCCode(v ?? '')}
          onMount={handleMount}
          theme="armv6"
          options={{
            fontSize: 13,
            fontFamily: "'JetBrains Mono', monospace",
            fontLigatures: true,
            lineNumbers: 'on',
            minimap: { enabled: true, scale: 1 },
            scrollBeyondLastLine: false,
            renderLineHighlight: 'gutter',
            cursorStyle: 'block',
            cursorBlinking: 'phase',
            smoothScrolling: true,
            bracketPairColorization: { enabled: true },
            padding: { top: 16, bottom: 16 },
            tabSize: 4,
            wordWrap: 'off',
            renderWhitespace: 'boundary',
            guides: { indentation: true, bracketPairs: true },
            suggest: { showKeywords: true },
            quickSuggestions: true,
            formatOnType: true,
          }}
        />
      </div>
    </div>
  );
}

// Minimal ELF32 parser for ARMv6 binaries

const ELF_MAGIC = [0x7F, 0x45, 0x4C, 0x46]; // \x7FELF

export interface ELFInfo {
  entryPoint: number;
  segments: ELFSegment[];
  sections: ELFSection[];
  architecture: string;
  isLittleEndian: boolean;
  valid: boolean;
  error?: string;
}

export interface ELFSegment {
  type: number;
  offset: number;
  vaddr: number;
  paddr: number;
  filesz: number;
  memsz: number;
  flags: number;
  data: Uint8Array;
}

export interface ELFSection {
  name: string;
  type: number;
  addr: number;
  offset: number;
  size: number;
}

function read32le(buf: Uint8Array, off: number): number {
  return (buf[off] | (buf[off+1] << 8) | (buf[off+2] << 16) | (buf[off+3] << 24)) >>> 0;
}
function read16le(buf: Uint8Array, off: number): number {
  return (buf[off] | (buf[off+1] << 8)) & 0xFFFF;
}

export function parseELF(bytes: Uint8Array): ELFInfo {
  // Check magic
  if (bytes.length < 52) return { valid: false, error: 'File too small', entryPoint: 0, segments: [], sections: [], architecture: '', isLittleEndian: true };
  for (let i = 0; i < 4; i++) {
    if (bytes[i] !== ELF_MAGIC[i]) return { valid: false, error: 'Not an ELF file (bad magic)', entryPoint: 0, segments: [], sections: [], architecture: '', isLittleEndian: true };
  }

  const elfClass = bytes[4]; // 1=32bit, 2=64bit
  if (elfClass !== 1) return { valid: false, error: 'Only ELF32 supported', entryPoint: 0, segments: [], sections: [], architecture: '', isLittleEndian: true };

  const endian = bytes[5]; // 1=LE, 2=BE
  if (endian !== 1) return { valid: false, error: 'Only little-endian ELF supported', entryPoint: 0, segments: [], sections: [], architecture: '', isLittleEndian: false };

  const machine = read16le(bytes, 18);
  const archMap: Record<number, string> = { 40: 'ARM', 183: 'AArch64', 62: 'x86-64', 3: 'x86' };
  const architecture = archMap[machine] ?? `Unknown (${machine})`;

  if (machine !== 40) return { valid: false, error: `Not an ARM binary (machine type: ${machine})`, entryPoint: 0, segments: [], sections: [], architecture, isLittleEndian: true };

  const entryPoint = read32le(bytes, 24);
  const phoff = read32le(bytes, 28);  // program header offset
  const shoff = read32le(bytes, 32);  // section header offset
  const phentsize = read16le(bytes, 42);
  const phnum = read16le(bytes, 44);
  const shentsize = read16le(bytes, 46);
  const shnum = read16le(bytes, 48);
  const shstrndx = read16le(bytes, 50);

  // Parse program headers (segments)
  const segments: ELFSegment[] = [];
  for (let i = 0; i < phnum; i++) {
    const off = phoff + i * phentsize;
    if (off + 32 > bytes.length) break;
    const type   = read32le(bytes, off + 0);
    const offset = read32le(bytes, off + 4);
    const vaddr  = read32le(bytes, off + 8);
    const paddr  = read32le(bytes, off + 12);
    const filesz = read32le(bytes, off + 16);
    const memsz  = read32le(bytes, off + 20);
    const flags  = read32le(bytes, off + 24);
    const data   = bytes.slice(offset, offset + filesz);
    segments.push({ type, offset, vaddr, paddr, filesz, memsz, flags, data });
  }

  // Parse section headers
  const sections: ELFSection[] = [];
  // First get string table
  let strTabData: Uint8Array | null = null;
  if (shstrndx < shnum && shoff > 0) {
    const soff = shoff + shstrndx * shentsize;
    if (soff + 40 <= bytes.length) {
      const strOff = read32le(bytes, soff + 16);
      const strSz  = read32le(bytes, soff + 20);
      strTabData = bytes.slice(strOff, strOff + strSz);
    }
  }

  function readCStr(buf: Uint8Array, off: number): string {
    let end = off;
    while (end < buf.length && buf[end] !== 0) end++;
    return new TextDecoder().decode(buf.slice(off, end));
  }

  for (let i = 0; i < shnum && shoff > 0; i++) {
    const off = shoff + i * shentsize;
    if (off + 40 > bytes.length) break;
    const nameIdx = read32le(bytes, off + 0);
    const type    = read32le(bytes, off + 4);
    const addr    = read32le(bytes, off + 12);
    const offset  = read32le(bytes, off + 16);
    const size    = read32le(bytes, off + 20);
    const name    = strTabData ? readCStr(strTabData, nameIdx) : `section_${i}`;
    sections.push({ name, type, addr, offset, size });
  }

  return { valid: true, entryPoint, segments, sections, architecture, isLittleEndian: true };
}

// Load ELF into CPU memory, return entry point
export function loadELFIntoMemory(elf: ELFInfo, memory: Uint8Array, memSize: number): number {
  const PT_LOAD = 1;
  for (const seg of elf.segments) {
    if (seg.type !== PT_LOAD) continue;
    const base = seg.vaddr & (memSize - 1);
    for (let i = 0; i < seg.filesz && base + i < memSize; i++) {
      memory[base + i] = seg.data[i];
    }
    // Zero out remaining memsz (BSS)
    for (let i = seg.filesz; i < seg.memsz && base + i < memSize; i++) {
      memory[base + i] = 0;
    }
  }
  return elf.entryPoint & (memSize - 1);
}

# ARMv6-M Emulator

A simple ARMv6-M (Cortex-M0 Thumb) emulator written in C++.

The emulator:
- Compiles C source code with vector tables
- Parses ELF binary for compiled C code
- Loads FLASH/RAM segments
- Emulates ARMv6-M Thumb instructions
- Provides a simple memory-mapped UART device

---

# Features

- ELF32 ARM firmware loading
- Cortex-M0 Thumb instruction execution
- Memory-mapped RAM/FLASH
- Basic stack + register emulation
- UART output emulation
- Freestanding firmware support

---

# Project Structure

```text
.
├── elf/
│   ├── linker.ld
│   ├── setup.c // Used for setting up vector tables for ARM based firmware
│   └── main.c
├── include/
├── src/
├── emulator // ELF executable emulator programer
└── CMakeLists.txt 

---

# Compilation

Generate build files:

```bash
mkdir build
cd build
make ..
cd ..
cmake --build build
```

---

# Usage

Run the emulator with a C source file:

```bash
./emulator [filename.c]
```

Example:

```bash
./emulator main.c
```

This will target the file inside the `/elf` folder, where it will be compiled using:
- `linker.ld`
- `setup.c`

Currently, the main example file is `main.c`.

---

# Debugging

All debug messages are written to `stderr`.
UART output are written to `stdio`.

Redirect debug output:

```bash
./emulator main.c 2> debug.log
```
Redirect stdio output:

```bash
./emulator main.c > output.txt
```


# Note

I haven't not yet fully implemented all the instructions, especially instructions involving special registers.

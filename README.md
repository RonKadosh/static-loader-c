# Static ELF32 Loader

---

## Overview

This project implements a **basic ELF32 loader** for Linux.

Developed as part of the Extended System Programming Lab at BGU.

It manually loads 32-bit ELF executables into memory by parsing their program headers, mapping segments appropriately, and transferring control to the program's entry point.  
The project is written in **C** and **x86 assembly**, and demonstrates low-level system programming concepts such as:

- Direct usage of system calls (`open`, `read`, `mmap`, etc.)
- Manual parsing of ELF headers and program headers
- Static memory mapping without relying on the OS dynamic loader
- Low-level startup sequence management

---

## Features

-  Parses **ELF32** files manually.
-  Loads program segments into appropriate memory addresses.
-  Transfers execution to the loaded program's **entry point**.
-  Custom startup routine written in x86 Assembly (`startup.s`, `start.s`).
-  Uses a **custom linker script** to control final layout.
-  Built with manual `ld` linking, specifying dynamic linker manually.

---

## Environment and Compilation

- **Operating System:** Linux
- **Architecture:** 32-bit ELF (**ELF32**)
- **C Compiler:** `gcc -m32`
- **Assembler:** `nasm -f elf32`
- **Linker:** `ld` with:
  - Library path: `/usr/lib32`
  - Dynamic linker: `/lib32/ld-linux.so.2`
  - Custom linker script: `linking_script`

---

### Prerequisites

Make sure your system has:

- `gcc-multilib` package installed (to compile 32-bit code on 64-bit systems).
- `lib32-glibc-dev` or equivalent 32-bit libc installed.
- `nasm` assembler installed.

Install missing packages (on Ubuntu/Debian):
```bash
sudo apt update
sudo apt install gcc-multilib libc6-dev-i386 nasm
```

## Build
```bash
make
````
This will:
-Compile loader.c into loader.o
-Assemble startup.s and start.s into startup.o and start.o
-Link everything using ld and custom linking_script

To run:
```bash
./loader <elf_file> [args...]
```

## Project Structure

| File           | Purpose                                           |
|----------------|---------------------------------------------------|
| `loader.c`     | Main C code for the loader                        |
| `startup.s`    | Assembly file setting up the environment (stack, registers) |
| `start.s`      | Additional assembly for program start logic       |
| `linking_script` | Custom linker script to control memory layout and sections |
| `Makefile`     | Build instructions                                |

---

## Notes
- linking_script was written by the Lab's TA.
- This loader is intended **only for 32-bit ELF binaries**.
- It **does not support 64-bit ELF64** files.
- It is mainly for **educational purposes**, demonstrating **manual program loading** and **low-level ELF file handling**.

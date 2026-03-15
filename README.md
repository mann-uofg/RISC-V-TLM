# RISC-V SystemC-TLM Simulator — Extended

This project extends the RISC-V SystemC-TLM simulator originally developed by
Màrius Montón. The base simulator models a RISC-V microcontroller in software
using SystemC TLM-2.0. This extended version adds memory-mapped UART and GPIO
peripherals, direct ELF binary loading, and a deterministic instruction tracer.

## Contributions Over Base Simulator

| Feature | Description |
|---|---|
| **UART Peripheral** | Memory-mapped UART at `0x40008000`. CPU writes chars via `sw` instruction; output appears on stdout |
| **GPIO Peripheral** | Memory-mapped GPIO at `0x4000C000`. Supports direction and data registers with per-pin state logging |
| **ELF Loader** | Simulator auto-detects and loads `.elf` binaries directly, eliminating the Intel HEX conversion step |
| **Instruction Tracer** | Logs every executed instruction (count, PC, encoding) to `trace.csv` for deterministic debugging |
| **Stdout Fix** | Replaced xterm-based Trace peripheral output with direct stdout, enabling headless/Docker usage |

---

## Prerequisites

- [Docker Desktop](https://www.docker.com/products/docker-desktop)
- Git

---

## Setup Instructions

### Step 1 — Clone the Repository
```bash
git clone https://github.com/mann-uofg/RISC-V-TLM.git
cd RISC-V-TLM
```

### Step 2 — Pull the Docker Image
```bash
docker pull mariusmm/riscv-tlm
```

### Step 3 — Start Docker Container
```bash
docker run -v "$(pwd):/project" -it mariusmm/riscv-tlm /bin/bash
```
> [!IMPORTANT]
> **macOS users:** If you get an invalid reference error, use:
> `docker run -v "$(pwd):/project" -it mariusmm/riscv-tlm /bin/bash`

### Step 4 — Install spdlog (inside Docker)
```bash
cd /project/ext/spdlog
mkdir -p build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local -DSPDLOG_BUILD_EXAMPLE=OFF
make -j4 && make install
cd /project
```

### Step 5 — Install RISC-V Toolchain (inside Docker)
```bash
wget --no-check-certificate \
  https://github.com/xpack-dev-tools/riscv-none-elf-gcc-xpack/releases/download/v13.2.0-2/xpack-riscv-none-elf-gcc-13.2.0-2-linux-x64.tar.gz

tar -xzf xpack-riscv-none-elf-gcc-13.2.0-2-linux-x64.tar.gz -C /usr/local/
export PATH="/usr/local/xpack-riscv-none-elf-gcc-13.2.0-2/bin:$PATH"
```

### Step 6 — Build the Simulator
```bash
cd /project
mkdir -p build && cd build
cmake .. && make -j4
cd /project
```

---

## Running the Tests

### Test 1 — UART Peripheral
```bash
# Compile test program
mkdir -p tests/C/uart_test
riscv-none-elf-gcc -Wall -O0 -nostdlib -march=rv32i \
  -mabi=ilp32 --entry main \
  tests/C/uart_test/main.c \
  -o tests/C/uart_test/uart_test.elf

# Run directly as ELF (ELF loader)
./build/RISCV_TLM tests/C/uart_test/uart_test.elf
```
**Expected output:**
```
ELF entry point: 0x10074
ELF loaded segment at 0x10000, size=0xda
Hello from UART!
ECALL Instruction called, stopping simulation
```

### Test 2 — GPIO Peripheral
```bash
riscv-none-elf-gcc -Wall -O0 -nostdlib -march=rv32i \
  -mabi=ilp32 --entry main tests/C/gpio_test/main.c \
  -o tests/C/gpio_test/gpio_test.elf

./build/RISCV_TLM tests/C/gpio_test/gpio_test.elf
```
**Expected output:**
```
[GPIO] Direction register set: 0xff
[GPIO] Output pins: 0x00000055
  Pin 0: HIGH  Pin 1: LOW ...
[GPIO] Output pins: 0x000000aa
  Pin 0: LOW   Pin 1: HIGH ...
```

### Test 3 — Instruction Tracer
After any simulation run, a `trace.csv` file is generated:
```bash
head -10 trace.csv
```
**Expected format:**
```
count,PC,instruction
0,0x00010074,0xfe010113
1,0x00010078,0x00812e23
...
```

---

## Project Structure

```
RISC-V-TLM/
├── src/
│   ├── UART.cpp          ← NEW: UART peripheral
│   ├── GPIO.cpp          ← NEW: GPIO peripheral
│   ├── Tracer.cpp        ← NEW: Instruction tracer
│   ├── Memory.cpp        ← MODIFIED: Added ELF loader
│   ├── Trace.cpp         ← MODIFIED: Stdout instead of xterm
│   ├── BusCtrl.cpp       ← MODIFIED: Added UART/GPIO routing
│   ├── CPU.cpp           ← MODIFIED: Tracer integration
│   └── Simulator.cpp     ← MODIFIED: UART/GPIO instantiation
├── inc/
│   ├── UART.h            ← NEW
│   ├── GPIO.h            ← NEW
│   ├── Tracer.h          ← NEW
│   └── ...
├── tests/
│   ├── C/uart_test/      ← NEW: UART test program
│   ├── C/gpio_test/      ← NEW: GPIO test program
│   └── ...
└── ext/
    └── spdlog/           ← Logging library
```

---

## Memory Map

| Address | Peripheral | Description |
|---|---|---|
| `0x00000000` – `0x0FFFFFFF` | RAM | Program and data memory |
| `0x40000000` | Trace | Original debug output (stdout) |
| `0x40004000` – `0x4000400C` | Timer | Hardware timer + compare registers |
| `0x40008000` | **UART TX** | Write char to transmit (NEW) |
| `0x4000C000` | **GPIO DIR** | Pin direction register (NEW) |
| `0x4000C004` | **GPIO DATA** | Pin data register (NEW) |

---

## Authors

- **Mann Modi** (1230348) — University of Guelph
- **Ritwij Gautam** (1172115) — University of Guelph
- Base simulator by Màrius Montón — [CARRV 2020](https://carrv.github.io/2020/papers/CARRV2020_paper_7_Monton.pdf)
```
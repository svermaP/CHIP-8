# CHIP-8 Emulator (C++ / SDL3)

A CHIP-8 emulator written in modern C++ using **SDL3** for rendering, input, and timing.  
Designed to be simple, correct, and explicit, with a clear fetch–decode–execute CPU model.

---

## Features

- Full CHIP-8 instruction set
- 4 KB memory model (programs loaded at `0x200`)
- 16 general-purpose registers (`V0–VF`), stack, and index register
- 60 Hz delay and sound timers (accumulator-based)
- Sprite rendering with collision detection (`VF`)
- Scaled 64×32 framebuffer via SDL streaming texture
- Keyboard input mapped to standard CHIP-8 layout
- Deterministic random number generation
- Blocking key instruction (`FX0A`) correctly implemented

---

## Controls

CHIP-8: 1 2 3 C Keyboard: 1 2 3 4
4 5 6 D Q W E R
7 8 9 E A S D F
A 0 B F Z X C V


---

## Build

### Requirements
- C++17+
- SDL3
- CMake
- Make

### Build Commands
```bash
make build
```

### Run (examples)
make ibm
make breakout
make flight
make snake
make tictactoe

## Run a custom ROM from the examples/ directory (without .ch8):
```bash
make custom ARG=rom_name
```

## Example
```bash
make custom ARG=PONG
```

### Notes
* Native resolution: 64×32, scaled 10×
* Timers tick independently at ~60 Hz
* CPU executes a fixed number of instructions per frame
* Fonts stored at memory address 0x050
* Rendering uses XOR draw semantics with proper collision flagging

## Help
```bash
make help
```
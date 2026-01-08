# CHIP-8 Emulator (C++ / SDL3)

A CHIP-8 emulator written in C++ using **SDL3** for rendering, input, and timing.  

Includes example games to run (run ```make help``` ) to view how

---

## Features

- CHIP-8 instruction set with most modern instructions
- 4 KB memory
- 16 general-purpose registers (`V0–VF`), stack, and index register
- 60 Hz delay and sound timers
- Scaled 64×32 display
- Keyboard input mapped to standard CHIP-8 layout
---

<h3>Controls</h3>

<p>CHIP-8 keypad → Host keyboard (QWERTY) </p>

<pre>
1 2 3 C    1 2 3 4
4 5 6 D    Q W E R
7 8 9 E    A S D F
A 0 B F    Z X C V
</pre>


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
```bash
make ibm
make breakout
make flight
make snake
make tictactoe
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

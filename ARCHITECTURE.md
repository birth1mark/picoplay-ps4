# PICO-8 PS4 Architecture & Technical Deep-Dive 🔬

**PICO-8 PS4** is a native, high-performance PICO-8 fantasy console interpreter and runtime environment engineered for the PlayStation 4 (via the OpenOrbis toolchain) and modern desktop systems.

This document serves as an in-depth technical manual detailing the internal memory architecture, virtual machine pipeline, audio synthesis, graphics subsystem, input abstraction, and cross-platform compilation target.

---

## 🏛️ System Overview

**PICO-8 PS4** operates by faithfully emulating the constraints, timing, and memory layout of the PICO-8 fantasy console while taking full advantage of the host hardware's native CPU and GPU capabilities.

```
┌─────────────────────────────────────────────────────────────┐
│                      PICO-8 Cartridge                       │
│           (.p8 text format or .p8.png steganography)        │
└──────────────────────────────┬──────────────────────────────┘
                               │
                               ▼
┌─────────────────────────────────────────────────────────────┐
│                      Cartridge Loader                       │
│         - LodePNG (steganographic RGBA bit-stripping)       │
│         - Miniz (decompressing PXA / compressed code)       │
│         - Lexer & Preprocessor (PICO-8 Lua dialect)         │
└──────────────────────────────┬──────────────────────────────┘
                               │
               ┌───────────────┴───────────────┐
               ▼                               ▼
┌─────────────────────────────┐ ┌─────────────────────────────┐
│      z8lua Virtual Machine  │ │      PicoRam (64 KB)        │
│  - 16.16 Fixed-Point Math   │ │  - 0x0000: Spritesheet      │
│  - Custom PICO-8 Syntax     │ │  - 0x2000: Map Memory       │
│  - Sandboxed Global API     │ │  - 0x3000: Flags & Audio    │
│  - Coroutine Scheduler      │ │  - 0x5F00: Draw/Screen Pal  │
│                             │ │  - 0x6000: Framebuffer (8KB)│
└──────────────┬──────────────┘ └──────────────┬──────────────┘
               │                               │
               └───────────────┬───────────────┘
                               │
                               ▼
┌─────────────────────────────────────────────────────────────┐
│                  Platform Host Abstraction                  │
│                        (IHost)                              │
│       ┌──────────────────────┴──────────────────────┐       │
│       ▼                                             ▼       │
│  [PS4Host]                                     [SDL2Host]   │
│  - OpenOrbis libkernel / pad                   - SDL2 Window│
│  - 1080p Hardware Scaler                       - Desktop GL │
│  - PS4 /data/ & USB Mounts                     - Local Path │
└─────────────────────────────────────────────────────────────┘
```

---

## 💾 Memory Architecture (`PicoRam`)

The PICO-8 virtual machine is defined by a strictly memory-mapped 64KB (0x10000) address space. In **PICO-8 PS4**, this is managed in `PicoRam.h` and accessed via fast inline pointer lookups and bitwise operations.

### Memory Map Layout

| Address Range | Size | Allocation / Hardware Purpose |
| :--- | :--- | :--- |
| `0x0000 - 0x0FFF` | 4,096 B | **Sprite Sheet (Sprites 0 - 127)**: 4 bits per pixel, 2 pixels per byte. |
| `0x1000 - 0x1FFF` | 4,096 B | **Shared Memory Area**: Lower sprite sheet (128 - 255) OR Bottom map row (32x64). |
| `0x2000 - 0x2FFF` | 4,096 B | **Map Memory**: 128x32 tilemap (1 byte per cell = sprite ID). |
| `0x3000 - 0x30FF` | 256 B | **Sprite Flags**: 8 flag bits per sprite ID (0 - 255). |
| `0x3100 - 0x313F` | 64 B | **Music Patterns**: 64 music track slots (4 channel indices per pattern). |
| `0x3200 - 0x42FF` | 4,352 B | **SFX Data**: 64 sound effects (32 notes each with pitch, waveform, volume, effect). |
| `0x4300 - 0x5EFF` | 7,168 B | **General Purpose / Custom User Memory**: Available for `peek()`, `poke()`, `memcpy()`. |
| `0x5F00 - 0x5F0F` | 16 B | **Draw Palette Remapping Table**: Maps virtual color indices to palette indices. |
| `0x5F10 - 0x5F1F` | 16 B | **Screen Palette Remapping Table**: Maps output colors to RGB output values. |
| `0x5F20 - 0x5F23` | 4 B | **Screen Clip Rectangle**: Left, Top, Right, Bottom bounds. |
| `0x5F24 - 0x5F27` | 4 B | **Draw State Modifiers**: Text color, fill pattern, camera offsets. |
| `0x5F28 - 0x5F2B` | 4 B | **Camera Coordinates**: 16-bit integer X and Y draw offsets. |
| `0x5F2C` | 1 B | **Screen Color Modes & Transformations** (e.g. mirrored mode, inverted). |
| `0x5F2D` | 1 B | **Devkit Keyboard / Mouse Mode Flag**. |
| `0x5F31 - 0x5F35` | 5 B | **Sound Hardware State Registers**. |
| `0x5F40 - 0x5F7F` | 64 B | **Extended Hardware State & Secondary Palette Remapping**. |
| `0x5F80 - 0x5FFF` | 128 B | **GPIO / Serial Communication Buffer**. |
| `0x6000 - 0x7FFF` | 8,192 B | **Screen Framebuffer**: 128x128 pixels, 4 bits per pixel (0.5 byte/pixel). |

### Framebuffer Bit-Packing
The screen is 128x128 pixels at 16 colors (indexed). Each byte in `0x6000 - 0x7FFF` stores two adjacent horizontal pixels:
- **Low 4 bits (0x0F):** Left pixel `(x, y)`
- **High 4 bits (0xF0):** Right pixel `(x + 1, y)`
- Calculation: `offset = 0x6000 + (y * 64) + (x >> 1)`

---

## ⚙️ The Scripting Engine: `z8lua` & 16.16 Fixed Point

PICO-8 does not use standard IEEE 754 floating-point arithmetic. Instead, it utilizes a custom **16.16 signed fixed-point integer number format (`fix32`)** to guarantee identical, deterministic behavior across all hardware platforms (preventing desyncs in replays or multiplayer).

### Fixed-Point Implementation
- Values range from `-32768.0` to `32767.9999847412109375`.
- Lowest representable fractional step: `1 / 65536 ≈ 0.0000152587890625`.
- In `z8lua`, arithmetic operations (multiplication, division, trigonometry) use 64-bit integer intermediates to prevent overflow before bit-shifting back to 16.16:
  ```cpp
  // Fixed-point multiplication concept in z8lua
  int32_t fix16_mul(int32_t a, int32_t b) {
      int64_t res = (int64_t)a * (int64_t)b;
      return (int32_t)(res >> 16);
  }
  ```

### PICO-8 Lua Syntactic Extensions
The Lua compiler in **PICO-8 PS4** contains a preprocessor and customized lexer that converts PICO-8 dialect extensions into standard executable bytecode:
1. **Shorthand `if` expressions:** `if (x < 10) x += 1 y -= 2` becomes `if (x < 10) then x = x + 1; y = y - 2; end`.
2. **Compound Assignment Operators:** `+=`, `-=`, `*=`, `/=`, `%=`, `&=`, `|=`, `^^=`, `<<=`, `>>=`.
3. **Unary Peek / Poke Shorthands:**
   - `@addr` ➔ `peek(addr)`
   - `%$addr` ➔ `peek2(addr)`
   - `^^addr` ➔ `peek4(addr)`
4. **Binary Integer Literals:** `0b101010` and hexadecimal notation `0x1f.8000`.
5. **Inequality Operator:** `!=` mapped to `~=`.

---

## 🎨 Graphics & Display Pipeline

The native PICO-8 canvas is 128x128 pixels. On the PlayStation 4, this is scaled to native 1080p (1920x1080) output using a dual-phase pipeline:

```
[ PicoRam 0x6000-0x7FFF ] (8 KB 4-bit indices)
          │
          ▼
[ Palette Translation ] ➔ Applies 0x5F00 (Draw) & 0x5F10 (Screen) palettes
          │
          ▼
[ 128x128 RGBA32 Texture ] (Host SDL2 / Video Memory)
          │
          ▼
[ Integer Scaling & Aspect Ratio Preservation ]
  - 8x integer scale = 1024x1024 active display area
  - 448px horizontal pillarboxes on 1920x1080 screen
  - Crisp nearest-neighbor interpolation (zero blur, authentic pixel art)
```

---

## 🔊 Audio Synthesis Engine (`Audio.cpp`)

**PICO-8 PS4** implements a real-time, 4-channel software synthesizer running at 22,050 Hz or 44,100 Hz output:

### 1. Channels & Polyphony
- **4 Simultaneous Monophonic Channels** (Channel 0 to 3).
- Each channel can be independently assigned to SFX or Music playback.

### 2. Built-in Waveforms (Oscillators)
| ID | Waveform | Acoustic Description |
| :--- | :--- | :--- |
| **0** | Sine / Triangle | Pure, smooth low frequency or melody tone |
| **1** | Tilted Sawtooth | Sharp, bright brassy timbre |
| **2** | Sawtooth | Harsh, classic 8-bit lead |
| **3** | Square | Hollow, retro chiptune staple (50% pulse duty) |
| **4** | Pulse | Thin, nasal sound (12.5% pulse duty) |
| **5** | Organ | Warm multi-harmonic blend |
| **6** | Noise | Pseudo-random white noise generator (percussion/explosions) |
| **7** | Phaser | Dual-frequency modulated bell / chime tone |

### 3. Note Effects
Each note can apply one of seven hardware modulations:
- `0: None`, `1: Slide`, `2: Vibrato`, `3: Drop`, `4: Fade In`, `5: Fade Out`, `6: Fast Arpeggio`, `7: Slow Arpeggio`.

---

## 🎮 Input Abstraction & DualShock 4 Mapping

Input is processed once per tick (60 Hz) inside `platform/ps4/source/PS4Host.cpp`:

```
DualShock 4 Physical Input              PICO-8 Virtual Bitmask (btn / btnp)
─────────────────────────────────────────────────────────────────────────────
D-Pad Left  / Left Stick Left   ───►    Bit 0 (0x01) : Left
D-Pad Right / Left Stick Right  ───►    Bit 1 (0x02) : Right
D-Pad Up    / Left Stick Up     ───►    Bit 2 (0x04) : Up
D-Pad Down  / Left Stick Down   ───►    Bit 3 (0x08) : Down
Cross (✕) / Square (▢)          ───►    Bit 4 (0x10) : Button 🅾 / Action 1
Circle (◯)                      ───►    Bit 5 (0x20) : Button ❎ / Action 2
Options Button                  ───►    Bit 6 (0x40) : Pause Menu
Triangle (△)                    ───►    Host Action  : Cycle Aspect Ratio
L1 + R1 + Options               ───►    Host Action  : Force Return to PS4 Menu
```

---

## 🌐 Network Subsystem & BBS Client (`source/BBSClient.cpp`)

To allow playing community cartridges without manual USB transfers, **PICO-8 PS4** includes an integrated BBS Downloader powered by Sony `SceHttp`/`SceSsl` on PS4 and `libcurl` on Desktop.

### Lexaloffle CDN Formula
Cartridges on the Lexaloffle BBS are stored deterministically on the public CDN using their unique Post ID / Local ID (`lid`):
- For numeric post IDs (e.g. `2145`):
  ```
  URL = https://www.lexaloffle.com/bbs/cposts/{id / 10000}/{id}.p8.png
  Example: https://www.lexaloffle.com/bbs/cposts/0/2145.p8.png
  ```
- For alphanumeric post IDs (e.g. `celeste_classic-0`):
  ```
  URL = https://www.lexaloffle.com/bbs/cposts/{lid[0..1]}/{lid}.p8.png
  ```

### Storage and Caching Pipeline
1. Client issues HTTP `GET` with header `User-Agent: PICO8-PS4/1.0 (PS4 Homebrew by birth1mark)`.
2. Validates HTTP 200 response and tests for the valid `\x89PNG` header signature.
3. Caches directly to `/data/pico8-ps4/carts/{lid}.p8.png` (PS4) or `~/p8carts/` (PC).
4. Reloads the cart list and hot-launches the game into memory.

### Lua Host API Bindings
**PICO-8 PS4** exposes dedicated secure C++ bindings to the Arcade BIOS Lua environment:
- `__bbs_listfeatured()`: Returns an indexed table of curated community games (`{id, title, author, cat, file, downloaded}`).
- `__bbs_download(lid, filename)`: Downloads a cartridge, verifies PNG magic bytes, and saves locally.
- `__bbs_isdownloaded(filename)`: Returns whether the given cartridge is already present on local storage.
- `__refreshcarts()`: Re-scans internal storage and USB mounts for newly acquired games.

---

## 🏗️ Build Targets & Toolchain Pipeline

### 1. OpenOrbis PS4 Toolchain
- **Compiler:** Clang/LLVM for `x86_64-pc-freebsd12-elf` with `-D__ORBIS__ -D__PS4__ -fPIC`.
- **Linker:** `ld.lld` linking with `-m elf_x86_64 -pie` against OpenOrbis `libc`, `libkernel`, `libScePad`, `libSceVideoOut`, `libSceAudioOut`, `libSceHttp`, and `libSDL2`.
- **Binary Conversion:** `create-fself` translates the ELF into a self-contained PlayStation 4 executable (`eboot.bin`).
- **Packaging:** `PkgTool.Core` and `tools/make_sfo.py` build the binary `param.sfo` containing metadata, Category `gd` (Game Application), and Title ID `PICO00008`, sealing `pico8-ps4.pkg`.

### 2. Desktop Target (Linux / PC)
- **Compiler:** `g++` / `clang++` with standard POSIX libraries and SDL2.
- **Window Management:** Creates an 800x800 resizable window centered on modern multi-monitor / ultra-wide setups.
- **Fast Development Cycle:** Compile and run in under 2 seconds: `make desktop && ./platform/SDL2Desktop/FAKE08`.

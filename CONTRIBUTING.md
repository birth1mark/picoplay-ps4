# Contributing to PICO-8 PS4 🤝

Welcome, fellow homebrew enthusiast! Thank you for taking an interest in contributing to **PICO-8 PS4**. Whether you are fixing an edge case in the `z8lua` interpreter, improving DualShock 4 controls, testing game compatibility, or submitting new starter cartridges, all contributions are warmly appreciated.

---

## 🎯 Code of Conduct & Philosophy

1. **Clean-Room Only:** **PICO-8 PS4** is 100% clean-room homebrew. **Never** submit code, headers, or reverse-engineered snippets derived from official Sony SDKs. We exclusively use the [OpenOrbis PS4 Toolchain](https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain) and open-source libraries.
2. **Respect the Creators:** PICO-8 is created by Joseph White (*zep* / Lexaloffle). Community cartridges are the creative work of their respective authors. Always respect licensing terms, give credit where due, and avoid adding non-permissive commercial ROMs.
3. **Keep It Lean & Fast:** PICO-8 games expect consistent 30/60 FPS frame rates without hitching. Prefer zero-allocation game loops and keep heap allocations during frame rendering to an absolute minimum.

---

## 🚀 Getting Started

### 1. Fork & Clone
```bash
git clone https://github.com/birth1mark/pico8-ps4.git
cd pico8-ps4
```

### 2. Local Desktop Development
To test and iterate quickly without turning on your PS4, compile the desktop SDL2 target:
```bash
# On Arch / EndeavourOS
sudo pacman -S sdl2 libcurl-gnutls

# Compile desktop build
make sdl2

# Run with a cartridge
./platform/SDL2Desktop/FAKE08 starter_carts/celeste.p8
```

### 3. PS4 Toolchain Build
To compile the PS4 executable (`eboot.bin`):
```bash
# Ensure OpenOrbis environment variable is set
export OPENORBIS=/opt/OpenOrbis/PS4-Toolchain

cd platform/ps4
make
```

---

## 🧪 Cartridge Compatibility Testing

One of the easiest and most valuable ways to contribute is testing PICO-8 cartridges and reporting issues:

1. **Test on both Desktop and PS4:**
   Verify if a cartridge behaves identically on desktop **PICO-8 PS4** and on hardware PS4.
2. **Audio & Visual Inspection:**
   - Are custom waveform SFX playing with the correct pitch and envelope?
   - Is map drawing (`map()`, `tline()`) rendering without seam glitches?
   - Are palette swap tricks (`pal()`, `palt()`) showing accurate transparency?
3. **Submitting a Bug Report:**
   Include:
   - Cartridge title and Lexaloffle BBS thread ID / URL.
   - PS4 firmware version (e.g. 9.00 / 11.00) or OS environment.
   - Exact steps to reproduce the glitch or error message.

---

## 🎨 Adding Pre-Bundled Starter Cartridges

To propose a new cartridge for the bundled `starter_carts/` collection:
- The cartridge must be released under an open-source or permissive Creative Commons license (e.g., CC BY-NC-SA, MIT), or explicitly granted for homebrew bundling by the original author.
- Ensure the author and license details are documented in `README.md` and `LEGAL.md`.
- Keep file sizes reasonable (standard `.p8` or `.p8.png` format under 32KB).

---

## 📬 Submitting a Pull Request (PR)

1. Create a feature branch:
   ```bash
   git checkout -b feature/amazing-feature
   ```
2. Commit your changes with clear, descriptive commit messages:
   ```bash
   git commit -m "PS4: Improve DualShock 4 analog stick deadzone handling"
   ```
3. Push to your fork:
   ```bash
   git push origin feature/amazing-feature
   ```
4. Open a Pull Request against `birth1mark/pico8-ps4` on GitHub.

Thank you for helping make **PICO-8 PS4** the best retro fantasy console experience on PlayStation 4! 🎮

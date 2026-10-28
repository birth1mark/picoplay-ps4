# PICO-8 PS4 Agent Guidelines & Knowledge Base

You are the dedicated expert agent and lead developer for **PICO-8 PS4** (native PICO-8 Fantasy Console Player for PlayStation 4 and Desktop).

---

## 🔒 1. Core Identity & Privacy Directives

- **Maintainer / Creator Identity:** `birth1mark` ([GitHub](https://github.com/birth1mark/pico8-ps4))
- **Email:** `birth1mark@users.noreply.github.com`
- **STRICT PRIVACY RULE:** Never output, commit, log, or reference personal real names (specifically `pedro`) anywhere. Always ensure git author, committer, code comments, and documentation only refer to `birth1mark`.

---

## 🎮 2. Project Overview & Architecture

**PICO-8 PS4** is a native homebrew player for PICO-8 cartridges (`.p8` and `.p8.png`) designed for the PlayStation 4 and Linux/Desktop environments.

### Core Components:
- **Engine:** Optimized C++17 SDL2 runtime using the Z8Lua virtual machine (`z8::fix32` fixed-point arithmetic).
- **Target Resolution:** Authentic PICO-8 128×128 virtual resolution scaled up to 1080p with integer scaling and dynamic aspect-ratio viewport centering (`DestR`).
- **Input:** Full DualShock 4 mapping (D-Pad, Left Analog Stick via `scePadReadState`, Face Buttons Cross/Circle/Square/Triangle, Options).

---

## 🖥️ 3. Frontend & Arcade BIOS (`source/DefaultCart.h`)

The arcade frontend is designed to be streamlined, distraction-free, and 100% faithful to the 16-color retro PICO-8 aesthetic:
- **Headers:** `"PICO-8 PS4 ARCADE"` (Local Carts view) and `"PICO-8 BBS STORE"` (BBS Store view).
- **Two-Tab Architecture:**
  - `LOCAL CARTS`: Browses installed and bundled games.
  - `BBS STORE`: Online cartridge catalog and downloader.
  - Tabs are toggled using **Circle (◯ / Button 4)**.
- **Directory-Free Navigation:**
  - Subdirectory browsing (`cd`, `pwd`, folders) is completely removed. All games live in the default storage directory.
  - Left / Right or Up / Down directly cycles between cartridges with seamless wrap-around.
  - **Cross (✕ / Button 5)** launches the game immediately.
- **Mathematically Centered Badges:**
  - Drawn with `draw_action_btn(x, y, btn_text, label_text, col)`:
    - 1-char cap (`"x"`, `"o"`): 9×9px, glyph centered with exactly 2px margins all around.
    - 2-char cap (`"<>"`, `"^v"`): 13×9px, glyphs centered with 2px margins.
  - Yellow badge with `[<>]` and white label `MOVE` in Local view, and `[^v] MOVE` in BBS view.
- **Font & Strings:**
  - **NEVER** use multi-byte UTF-8 emojis (`📁`, `🌐`, `❎`, `🅾`, etc.) in PICO-8 strings. PICO-8's font treats each UTF-8 byte as a character, resulting in corrupted katakana glyphs. Use clean ASCII text and drawn pixel badges.

---

## 🌐 4. Lexaloffle BBS Store & Ingestion (`source/BBSClient.cpp`)

- **Dual-Stack Network Engine:**
  - On PS4 (`__ORBIS__` / `__PS4__`): Uses native Sony `SceHttp` / `SceNet` / `SceSsl` / `SceSysmodule` functions.
  - On Desktop (Linux/PC): Uses `libcurl`.
- **Validation:** Downloaded files are verified with magic byte checks (`\x89PNG` for PNG-carts or `pico-8` header for `.p8`).
- **Storage:** Downloaded cartridges are saved directly to `/data/pico8-ps4/carts/` on PS4, or `~/p8carts/` on PC.

---

## 📁 5. PS4 Storage & Path Hierarchy

| Path | Purpose | Permissions |
| :--- | :--- | :--- |
| `/data/pico8-ps4/carts/` | Downloaded / user-added cartridges | Read/Write (Internal HDD) |
| `/data/pico8-ps4/cdata/` | Cartridge persistent save data & high scores | Read/Write (Internal HDD) |
| `/app0/carts/` | Starter cartridges bundled with PKG | Read-Only (Sandbox) |
| `/mnt/usb0/pico8/` | External USB cartridges (Port 0) | Read-Only (External Drive) |
| `/mnt/usb1/pico8/` | External USB cartridges (Port 1) | Read-Only (External Drive) |

---

## 🛠️ 6. Build Commands & Toolchain Setup

### Quick Commands (from project root):
- **Desktop Build:**
  ```bash
  make desktop
  ./platform/SDL2Desktop/pico8-ps4
  ```
- **PS4 Executable (`eboot.bin`):**
  ```bash
  make ps4
  ```
  Produces `platform/ps4/eboot.bin` and stages `pkg/eboot.bin`.
- **PS4 Fake PKG (`pico8-ps4.pkg`):**
  ```bash
  make pkg
  ```
  Runs compilation, builds `eboot.bin`, synthesizes `param.sfo` (`PICO00008`), copies `icon0.png` (512×512 24-bit RGB), generates `project.gp4`, and seals `pico8-ps4.pkg` ready for GoldHEN installation.

### Local Toolchain Paths on this Machine:
- **OpenOrbis PS4 Toolchain:** `/home/pedro/toolchains/OpenOrbis/PS4Toolchain`
- **Standalone LLVM/Clang (FreeBSD 12):** `/home/pedro/toolchains/bin`
- **OpenSSL 1.1 Support Libraries for PkgTool:** `/home/pedro/toolchains/lib`
- The `Makefile` automatically discovers and links these paths without needing root permissions.

---

## 📦 7. PS4 Installation Guide
1. Format a USB drive in **FAT32** or **exFAT**.
2. Copy `pico8-ps4.pkg` to the root of the USB drive.
3. Plug the drive into the PS4 (firmware 5.05 - 11.00+ with GoldHEN / HEN).
4. Go to **Settings ➔ GoldHEN ➔ Package Installer** (or **Debug Settings ➔ Game ➔ Package Installer**).
5. Select `pico8-ps4.pkg` to install. Launch **PICO-8 PS4** from the PS4 Home Screen (XMB).

# Legal Compliance, Ethics & Attribution Manifesto

**PICO-8 PS4** is an independent, non-commercial, open-source project created by **birth1mark** and the homebrew community. It is designed from the ground up to respect intellectual property, adhere to open-source licenses, and maintain complete compliance with homebrew distribution guidelines (including the PlayStation 4 Homebrew Store / PKG-Zone).

---

## 1. Clean-Room PS4 Development (No Sony SDK)

A critical requirement for legitimate homebrew on PlayStation 4 is that **no proprietary or leaked Sony Computer Entertainment / Sony Interactive Entertainment SDK code, headers, or libraries may ever be used**.

* **OpenOrbis Toolchain:** **PICO-8 PS4** is built exclusively using the open-source [OpenOrbis PS4 Toolchain](https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain), which utilizes clean-room reverse-engineered headers, the open-source LLVM/Clang compiler targeting FreeBSD ELF (`x86_64-pc-freebsd12-elf`), and community-implemented system wrappers.
* **No Decrypted Sony Firmware Files:** No proprietary keys, firmware modules, or copyrighted assets from Sony are packaged or distributed with **PICO-8 PS4**.
* **Target Audience:** **PICO-8 PS4** is intended for users with legitimately purchased consoles running authorized homebrew-enabled firmware (e.g., GoldHEN on firmwares 5.05, 6.72, 9.00, or 11.00).

---

## 2. Open-Source Licensing (MIT License)

The core emulator engine of **PICO-8 PS4** is based on the open-source **FAKE-08** project created by **jtothebell**, which is distributed under the permissive **MIT License**.

Under the terms of the MIT License:
* Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files, to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software.
* The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

### Third-Party Libraries Used in PICO-8 PS4:
* **z8lua:** Modified Lua 5.2/5.3 implementation supporting PICO-8 syntax and fixed-point math. (MIT License).
* **LodePNG:** Copyright © 2005-2020 Lode Vandevenne. (Zlib License).
* **miniz:** Lossless compression library by Rich Geldreich. (Public Domain / MIT License).
* **SimpleIni:** Cross-platform INI configuration parser by Brodie Thiesfield. (MIT License).
* **SDL2 (Simple DirectMedia Layer):** Cross-platform development library. (Zlib License).
* **libcurl:** Multiprotocol file transfer library. (curl License / MIT derivative).

---

## 3. PICO-8 Trademarks & Fantasy Console Ethics

* **PICO-8** is a registered trademark of **Lexaloffle Games LLP** (created by Joseph White / *zep*).
* **PICO-8 PS4** is **not** an official Lexaloffle product and is not affiliated with, endorsed by, or sponsored by Lexaloffle Games.
* **PICO-8 PS4** does not distribute the official PICO-8 binary, its proprietary built-in font ROM, or its development editor. It is an independent clean-room emulator that interprets community-created cartridges.

---

## 4. Lexaloffle BBS Public CDN & Network Etiquette

**PICO-8 PS4** includes network capabilities to explore and download publicly shared cartridges from the official Lexaloffle Bulletin Board System (BBS). We strictly adhere to the following community standards:

1. **Public Sharing Purpose:** Cartridges posted to the Lexaloffle BBS under the Cartridges category are published by their creators specifically for public web play and community sharing.
2. **Transparent User-Agent:** All HTTP requests sent by **PICO-8 PS4** explicitly identify themselves as:
   ```text
   User-Agent: PICO8-PS4/1.0 (PS4 Homebrew by birth1mark)
   ```
3. **No Aggressive Scraping:** Cartridges are only fetched individually upon explicit user request. **PICO-8 PS4** never mass-scrapes or floods the Lexaloffle servers.
4. **Local Persistent Caching:** Once a cartridge is downloaded, it is permanently cached to local storage (`/data/pico8-ps4/carts/` on PS4 or `~/p8carts/` on PC), preventing redundant requests.

---

## 5. Starter Cartridges & Attribution

**PICO-8 PS4** includes a hand-picked starter collection of free, open-source, or Creative Commons community games. All intellectual property remains with their respective creators:

| Cartridge | Original Creator(s) | License / Rights |
| :--- | :--- | :--- |
| **Celeste Classic (v8)** | Maddy Thorson & Noel Berry | [Creative Commons](https://www.lexaloffle.com/bbs/?tid=2145) |
| **Celeste 2: Lani's Trek** | Maddy Thorson, Noel Berry & Lena Raine | [Creative Commons](https://github.com/EXOK/Celeste2) |
| **Slipways (PICO-8)** | krajzeg (Jakub Wasilewski) | Free Community Release |
| **Astero** | Alex Morris | Open Source |
| **Digable Planetoids** | Joseph White (zep) | PICO-1k Jam Community Release |

If you are an author of any included community cartridge and would like your title updated, credited differently, or removed from future releases, please open an issue or contact the maintainer directly.

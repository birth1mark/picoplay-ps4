#!/usr/bin/env python3
"""
PicoPlay BBS Cartridge Explorer & Downloader
Allows searching, browsing and downloading public PICO-8 carts from Lexaloffle BBS.
"""
import sys
import os
import re
import urllib.request

USER_AGENT = "PicoPlay/1.0 (Homebrew by birth1mark)"
DEST_DIR = os.path.expanduser("~/p8carts")

def get_cart_url(lid):
    lid = str(lid).strip()
    if lid.isdigit():
        folder = str(int(lid) // 10000)
    else:
        folder = lid[:2]
    return f"https://www.lexaloffle.com/bbs/cposts/{folder}/{lid}.p8.png"

def download_cart(lid, filename=None):
    os.makedirs(DEST_DIR, exist_ok=True)
    if not filename:
        clean_name = re.sub(r'[^a-zA-Z0-9_\-]', '_', lid)
        filename = f"{clean_name}.p8.png"
    
    dest_path = os.path.join(DEST_DIR, filename)
    url = get_cart_url(lid)
    
    print(f"[*] Downloading from Lexaloffle BBS: {url}")
    req = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
    try:
        with urllib.request.urlopen(req, timeout=10) as resp, open(dest_path, "wb") as f:
            data = resp.read()
            f.write(data)
        print(f"[✓] Saved successfully to: {dest_path} ({len(data)} bytes)")
        print(f"[✓] Game is now ready in your PicoPlay library!")
        return True
    except Exception as e:
        print(f"[✗] Failed to download {lid}: {e}")
        return False

def list_recent():
    url = "https://www.lexaloffle.com/bbs/feed.php?cat=7&sub=2"
    print(f"[*] Fetching latest cartridges from Lexaloffle BBS...")
    req = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
    try:
        with urllib.request.urlopen(req, timeout=10) as resp:
            xml = resp.read().decode("utf-8", errors="ignore")
        
        items = re.findall(r"<item>(.*?)</item>", xml, re.DOTALL)
        print(f"\n{'TITLE':<35} {'AUTHOR':<18} {'LID / CODE'}")
        print("-" * 75)
        for item in items[:12]:
            title = (re.search(r"<title>(.*?)</title>", item) or [None, "Unknown"])[1]
            author = (re.search(r"by <a [^>]*>(.*?)</a>", item) or [None, "Community"])[1].strip()
            thumb = (re.search(r"/bbs/thumbs/(?:pico8_|pico64_)?([a-zA-Z0-9_\-]+)\.png", item) or [None, "N/A"])[1]
            print(f"{title[:34]:<35} {author[:17]:<18} {thumb}")
        print("\nTo download any of these to your PicoPlay library, run:")
        print("  python3 tools/get_pico.py get <LID>")
    except Exception as e:
        print(f"[✗] Error fetching feed: {e}")

if __name__ == "__main__":
    if len(sys.argv) < 2 or sys.argv[1] == "list":
        list_recent()
    elif sys.argv[1] == "get" and len(sys.argv) > 2:
        download_cart(sys.argv[2])
    else:
        print("Usage:")
        print("  python3 tools/get_pico.py list         - List recent community games")
        print("  python3 tools/get_pico.py get <LID>    - Download game by LID to PicoPlay")

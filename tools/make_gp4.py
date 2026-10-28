#!/usr/bin/env python3
"""
Generates project.gp4 for PICO-8 PS4 PKG creation.
Scans pkg/ directory for eboot.bin, sce_sys files, and bundled starter carts.
"""
import os
import glob
import sys

def generate_gp4(pkg_dir="pkg", title_id="PICO00008"):
    content_id = f"IV0000-{title_id}_00-PICO8PS4PLAYER00"
    
    files = []
    # Standard mandatory OpenOrbis PS4 PKG files
    for base in [
        "eboot.bin",
        "sce_sys/about/right.sprx",
        "sce_sys/param.sfo",
        "sce_sys/icon0.png",
        "sce_module/libc.prx",
        "sce_module/libSceFios2.prx",
    ]:
        path = os.path.join(pkg_dir, base)
        if os.path.exists(path):
            files.append(base)
            
    # Bundled carts
    carts = sorted(glob.glob(os.path.join(pkg_dir, "carts", "*")))
    for cart in carts:
        if os.path.isfile(cart):
            files.append(os.path.relpath(cart, pkg_dir))

    lines = [
        '<?xml version="1.0"?>',
        '<psproject xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" fmt="gp4" version="1000">',
        '\t<volume>',
        '\t\t<volume_type>pkg_ps4_app</volume_type>',
        '\t\t<volume_id>PS4VOLUME</volume_id>',
        '\t\t<volume_ts>2026-10-23 10:00:00</volume_ts>',
        f'\t\t<package content_id="{content_id}" passcode="00000000000000000000000000000000"',
        '\t\t\tstorage_type="digital50" app_type="full" />',
        '\t\t<chunk_info chunk_count="1" scenario_count="1">',
        '\t\t\t<chunks>',
        '\t\t\t\t<chunk id="0" layer_no="0" label="Chunk #0" />',
        '\t\t\t</chunks>',
        '\t\t\t<scenarios default_id="0">',
        '\t\t\t\t<scenario id="0" type="sp" initial_chunk_count="1" label="Scenario #0">0</scenario>',
        '\t\t\t</scenarios>',
        '\t\t</chunk_info>',
        '\t</volume>',
        '\t<files img_no="0">'
    ]

    for f in files:
        lines.append(f'\t\t<file targ_path="{f}" orig_path="{f}" />')

    lines.extend([
        '\t</files>',
        '\t<rootdir>',
        '\t\t<dir targ_name="sce_sys">',
        '\t\t\t<dir targ_name="about" />',
        '\t\t</dir>',
        '\t\t<dir targ_name="sce_module" />',
        '\t\t<dir targ_name="carts" />',
        '\t</rootdir>',
        '</psproject>'
    ])

    gp4_path = os.path.join(pkg_dir, "project.gp4")
    with open(gp4_path, "w", encoding="utf-8") as out:
        out.write("\n".join(lines) + "\n")
    print(f"Generated {gp4_path} with {len(files)} files.")

if __name__ == "__main__":
    pkg_dir = sys.argv[1] if len(sys.argv) > 1 else "pkg"
    generate_gp4(pkg_dir)

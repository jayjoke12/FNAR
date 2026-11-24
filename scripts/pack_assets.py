#!/usr/bin/env python3
import os
import sys
import struct

def pack_assets(source_dirs, output_path):
    bundle = bytearray()
    bundle.extend(b'FNAF')
    bundle.extend(struct.pack('<I', 1)) # Version 1

    files_to_pack = []
    for sdir in source_dirs:
        if not os.path.exists(sdir):
            continue
        sdir_abs = os.path.abspath(sdir)
        base_dir = os.path.dirname(sdir_abs)
        for root, _, files in os.walk(sdir_abs):
            for f in sorted(files):
                full_path = os.path.join(root, f)
                rel_path = os.path.relpath(full_path, base_dir).replace('\\', '/').lstrip('/')
                files_to_pack.append((rel_path, full_path))

    bundle.extend(struct.pack('<I', len(files_to_pack)))
    for rel_path, full_path in files_to_pack:
        rel_bytes = rel_path.encode('utf-8')
        # Preserve each file's actual encoding. Raylib selects its image decoder
        # from the bundled path extension, so JPEG bytes stored as .png fail to load.
        with open(full_path, 'rb') as f:
            data = f.read()

        bundle.extend(struct.pack('<H', len(rel_bytes)))
        bundle.extend(rel_bytes)
        bundle.extend(struct.pack('<I', len(data)))
        bundle.extend(data)

    os.makedirs(os.path.dirname(os.path.abspath(output_path)), exist_ok=True)
    with open(output_path, 'wb') as f:
        f.write(bundle)

    print(f"[Assets Pack] Packed {len(files_to_pack)} files ({len(bundle)/1024/1024:.2f} MB) -> {output_path}")

if __name__ == '__main__':
    if len(sys.argv) < 3:
        print("Usage: pack_assets.py <output_bundle> <dir1> [dir2 ...]")
        sys.exit(1)
    pack_assets(sys.argv[2:], sys.argv[1])

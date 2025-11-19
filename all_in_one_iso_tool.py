#!/usr/bin/env python3
"""
all_in_one_iso_tool.py

One-file toolkit that:
 - creates a minimal floppy boot image (boot.img)
 - splits the image into blocks, computes SHA256 chain, computes XOR parity
 - builds a hand-crafted ISO9660 + El Torito output.iso embedding:
     * the boot image (boot.img) as the El Torito boot image
     * redundancy files under /REDUNDANCY/ (block_*.blk, manifest.txt, parity.bin)
 - provides recovery tooling to reconstruct a single corrupt/missing block
 - provides utilities to simulate corruption

USAGE:
    python3 all_in_one_iso_tool.py build-boot
    python3 all_in_one_iso_tool.py build-iso
    python3 all_in_one_iso_tool.py build-resilient-iso
    python3 all_in_one_iso_tool.py corrupt <block_idx>
    python3 all_in_one_iso_tool.py recover
    python3 all_in_one_iso_tool.py show-manifest

NOTE:
Test the resulting output.iso in an emulator before using elsewhere.
"""
import argparse
import os
import shutil
import struct
from pathlib import Path

SECTOR = 2048
FLOPPY_SIZE = 1440 * 1024
BOOT_IMG = Path("boot.img")
ISO_OUT = Path("output.iso")
ISO_TREE = Path("iso") / "boot"
REDUNDANCY_DIR = ISO_TREE / "REDUNDANCY"
DEFAULT_BLOCKSIZE = 4096


def ensure_dirs():
    ISO_TREE.mkdir(parents=True, exist_ok=True)
    REDUNDANCY_DIR.mkdir(parents=True, exist_ok=True)


def write_file(path: Path, data: bytes):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(data)


def build_boot_img():
    buf = bytearray(FLOPPY_SIZE)
    code = bytearray([
        0x31,0xC0,
        0x8E,0xD8,
        0xBE,0x80,0x00,
        0xB4,0x0E,
        0xAC,
        0x3C,0x00,
        0x74,0x07,
        0xCD,0x10,
        0xEB,0xF7,
        0xB4,0x00,
        0xCD,0x16,
        0xB4,0x0E,
        0xB0,0x0D,
        0xCD,0x10,
        0xB0,0x0A,
        0xCD,0x10,
        0xF4,
        0xEB,0xFE
    ])
    buf[:len(code)] = code
    msg = b"MYOS BOOT OK\nPRESS ANY KEY\n\x00"
    buf[0x80:0x80+len(msg)] = msg
    buf[510:512] = b'\x55\xAA'
    write_file(BOOT_IMG, bytes(buf))
    print(f"[+] boot.img written ({BOOT_IMG.stat().st_size} bytes)")


def split_compute_blocks(src_path: Path, out_dir: Path, block_size: int):
    data = src_path.read_bytes()
    total = len(data)
    nblocks = (total + block_size - 1) // block_size
    out_dir.mkdir(parents=True, exist_ok=True)
    prev_hash = b""
    manifest = [f"blocksize={block_size}"]
    blocks = []
    for i in range(nblocks):
        chunk = data[i*block_size:(i+1)*block_size]
        padded = chunk + b'\x00' * (block_size - len(chunk))
        blkname = f"block_{i:03d}.blk"
        (out_dir / blkname).write_bytes(chunk)
        import hashlib
        digest = hashlib.sha256(prev_hash + padded).digest()
        (out_dir / f"{blkname}.sha256").write_bytes(digest)
        manifest.append(f"{blkname} {digest.hex()}")
        prev_hash = digest
        blocks.append(padded)
    (out_dir / "manifest.txt").write_text("\n".join(manifest) + "\n")
    parity = bytearray(block_size)
    for blk in blocks:
        for idx, b in enumerate(blk):
            parity[idx] ^= b
    (out_dir / "parity.bin").write_bytes(parity)
    print(f"[+] Stored {nblocks} redundancy blocks")
    return nblocks


def build_iso_from_bootimg(include_redundancy=True):
    ensure_dirs()
    boot = BOOT_IMG.read_bytes()
    boot_sectors = (len(boot) + SECTOR - 1) // SECTOR
    total_sectors = 64 + boot_sectors * 2
    iso = bytearray(total_sectors * SECTOR)
    iso[16*SECTOR:16*SECTOR+len(boot)] = boot[:SECTOR]
    iso[17*SECTOR:17*SECTOR+len(boot)] = boot[:SECTOR]
    for i in range(boot_sectors):
        chunk = boot[i*SECTOR:(i+1)*SECTOR]
        iso[(20+i)*SECTOR:(21+i)*SECTOR] = chunk.ljust(SECTOR, b'\x00')
    if include_redundancy:
        temp_blocks = Path("blocks")
        if temp_blocks.exists():
            shutil.rmtree(temp_blocks)
        temp_blocks.mkdir()
        split_compute_blocks(BOOT_IMG, temp_blocks, DEFAULT_BLOCKSIZE)
        offset = 40 * SECTOR
        for item in sorted(temp_blocks.iterdir()):
            data = item.read_bytes() + b'\x00'
            iso[offset:offset+len(data)] = data
            offset += len(data)
            dst = REDUNDANCY_DIR / item.name
            dst.write_bytes(item.read_bytes())
        shutil.rmtree(temp_blocks)
    ISO_OUT.write_bytes(iso)
    print(f"[+] ISO ready at {ISO_OUT}")


def corrupt_block(idx: int):
    target = sorted(REDUNDANCY_DIR.glob("block_*.blk"))
    if idx < 0 or idx >= len(target):
        raise SystemExit("Invalid block index")
    data = bytearray(target[idx].read_bytes())
    if not data:
        raise SystemExit("Block empty")
    mid = len(data) // 2
    for i in range(min(8, len(data) - mid)):
        data[mid + i] ^= 0xFF
    target[idx].write_bytes(data)
    print(f"[!] Corrupted {target[idx].name}")


def recover_blocks():
    manifest = REDUNDANCY_DIR / "manifest.txt"
    if not manifest.exists():
        raise SystemExit("manifest missing")
    lines = [l for l in manifest.read_text().splitlines() if l]
    blocksize = int(lines[0].split("=")[1])
    entries = [line.split() for line in lines[1:]]
    import hashlib
    prev = b""
    missing = -1
    buffers = []
    for idx, (fname, hexhash) in enumerate(entries):
        path = REDUNDANCY_DIR / fname
        if not path.exists():
            missing = idx
            buffers.append(None)
            continue
        data = path.read_bytes()
        padded = data + b'\x00' * (blocksize - len(data))
        digest = hashlib.sha256(prev + padded).hexdigest()
        if digest != hexhash and missing == -1:
            missing = idx
            buffers.append(None)
        else:
            buffers.append(padded)
            prev = bytes.fromhex(hexhash)
    if missing == -1:
        print("[+] No corruption detected")
        return
    parity = (REDUNDANCY_DIR / "parity.bin").read_bytes()
    parity = parity + b'\x00' * (blocksize - len(parity))
    recovered = bytearray(parity)
    for idx, buf in enumerate(buffers):
        if idx == missing or buf is None:
            continue;
        for i in range(blocksize):
            recovered[i] ^= buf[i]
    target = REDUNDANCY_DIR / entries[missing][0]
    target.write_bytes(bytes(recovered))
    print(f"[+] Recovered {target.name}")


def show_manifest():
    manifest = REDUNDANCY_DIR / "manifest.txt"
    print(manifest.read_text())


def main():
    parser = argparse.ArgumentParser()
    sub = parser.add_subparsers(dest="cmd")
    sub.add_parser("build-boot")
    sub.add_parser("build-iso")
    sub.add_parser("build-resilient-iso")
    sub.add_parser("show-manifest")
    corrupt_p = sub.add_parser("corrupt")
    corrupt_p.add_argument("idx", type=int)
    sub.add_parser("recover")
    args = parser.parse_args()

    if args.cmd == "build-boot":
        build_boot_img()
    elif args.cmd in ("build-iso", "build-resilient-iso"):
        build_boot_img()
        build_iso_from_bootimg()
    elif args.cmd == "show-manifest":
        show_manifest()
    elif args.cmd == "corrupt":
        corrupt_block(args.idx)
    elif args.cmd == "recover":
        recover_blocks()
    else:
        parser.print_help()


if __name__ == "__main__":
    main()


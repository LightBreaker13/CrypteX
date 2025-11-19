#!/usr/bin/env python3
"""
Helper script for building and optionally running the MyOS ISO image.
"""
import argparse
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent
ISO = ROOT / "myos.iso"


def check_tool(name: str) -> None:
    if shutil.which(name) is None:
        raise SystemExit(f"[!] Required tool '{name}' not found in PATH.")


def run(cmd: str) -> None:
    print(f"[+] {cmd}")
    subprocess.run(cmd, shell=True, check=True)


def clean() -> None:
    shutil.rmtree(ROOT / "build", ignore_errors=True)
    shutil.rmtree(ROOT / "iso", ignore_errors=True)
    if ISO.exists():
        ISO.unlink()
    print("[✓] Clean complete.")


def main() -> None:
    parser = argparse.ArgumentParser(description="Build helper for MyOS.")
    parser.add_argument("--run", action="store_true", help="Launch QEMU after build.")
    parser.add_argument("--clean", action="store_true", help="Remove build artifacts.")
    parser.add_argument("--manual", action="store_true", help="Skip Makefile and run manual ISO builder.")
    args = parser.parse_args()

    if args.clean:
        clean()
        return

    check_tool("gcc")
    check_tool("ld")
    check_tool("grub-mkrescue")
    check_tool("qemu-system-i386")

    if args.manual:
        raise SystemExit("[!] Manual build path not implemented; use Makefile workflow.")

    run("make iso")

    if args.run:
        run("qemu-system-i386 -cdrom myos.iso -m 512 -display sdl")


if __name__ == "__main__":
    main()


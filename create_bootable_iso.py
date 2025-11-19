#!/usr/bin/env python3
"""
create_bootable_iso.py

Creates a bootable GRUB ISO for VirtualBox.
This script creates a proper ISO9660 structure with GRUB bootloader.
"""
import os
import sys
import struct
import subprocess
import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parent
ISO_OUT = ROOT / "myos_bootable.iso"
ISO_DIR = ROOT / "iso_grub"
BUILD_DIR = ROOT / "build"

def create_iso_structure():
    """Create proper GRUB ISO directory structure."""
    print("[*] Creating ISO directory structure...")
    
    # Clean and create directories
    if ISO_DIR.exists():
        shutil.rmtree(ISO_DIR)
    ISO_DIR.mkdir(parents=True)
    
    boot_dir = ISO_DIR / "boot"
    grub_dir = boot_dir / "grub"
    grub_dir.mkdir(parents=True)
    
    # Create GRUB config
    grub_cfg = grub_dir / "grub.cfg"
    grub_cfg.write_text("""set timeout=0
set default=0

menuentry "MyOS" {
    multiboot2 /boot/kernel.bin
    boot
}
""")
    
    # Try to copy or create kernel
    kernel_src = BUILD_DIR / "kernel.bin"
    if kernel_src.exists():
        shutil.copy2(kernel_src, boot_dir / "kernel.bin")
        print(f"[+] Using kernel from build: {kernel_src}")
    else:
        # Create a minimal placeholder (will be replaced if build succeeds)
        print("[!] No kernel.bin found, creating placeholder...")
        placeholder = boot_dir / "kernel.bin"
        # Minimal valid ELF structure
        elf_data = (
            b'\x7fELF\x01\x01\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00'  # ELF header start
            b'\x02\x00\x03\x00\x01\x00\x00\x00\x00\x10\x00\x00'  # ELF header cont
            b'\x34\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'  # ELF header cont
            b'\x34\x00\x20\x00\x01\x00\x00\x00\x00\x00\x00\x00'  # ELF header cont
            b'\x00\x00\x00\x00\x00\x00\x00\x00'  # Program header
            b'\x00\x00\x00\x00\x00\x00\x00\x00'  # Program header cont
            b'\x00\x00\x00\x00\x00\x00\x00\x00'  # Program header cont
            b'\x00\x00\x00\x00\x00\x00\x00\x00'  # Program header cont
        )
        placeholder.write_bytes(elf_data)
        print("[!] Placeholder created. Build kernel first with: make iso")
    
    return True

def build_with_grub_mkrescue():
    """Try to build using grub-mkrescue."""
    if not shutil.which("grub-mkrescue"):
        return False
    
    print("[*] Building ISO with grub-mkrescue...")
    try:
        cmd = ["grub-mkrescue", "-o", str(ISO_OUT), str(ISO_DIR)]
        result = subprocess.run(cmd, capture_output=True, text=True)
        if result.returncode == 0:
            print(f"[+] ISO created successfully: {ISO_OUT}")
            return True
        else:
            print(f"[!] grub-mkrescue error: {result.stderr}")
            return False
    except Exception as e:
        print(f"[!] grub-mkrescue failed: {e}")
        return False

def build_with_xorriso():
    """Try to build using xorriso."""
    if not shutil.which("xorriso"):
        return False
    
    print("[*] Building ISO with xorriso...")
    try:
        # First, we need GRUB boot files
        # Try to find them
        grub_prefixes = [
            "/usr/lib/grub/i386-pc",
            "/usr/share/grub/i386-pc",
            "C:\\grub\\i386-pc",  # Windows path
        ]
        
        boot_img = None
        for prefix in grub_prefixes:
            test_path = Path(prefix) / "boot.img"
            if test_path.exists():
                boot_img = test_path
                break
        
        if not boot_img:
            print("[!] GRUB boot.img not found. Cannot create bootable ISO.")
            print("[!] Please install GRUB or use WSL/Linux.")
            return False
        
        cmd = [
            "xorriso", "-as", "mkisofs",
            "-R", "-J", "-joliet-long",
            "-b", "boot/grub/i386-pc/boot.img",
            "-no-emul-boot", "-boot-load-size", "4",
            "-boot-info-table",
            "-o", str(ISO_OUT),
            str(ISO_DIR)
        ]
        result = subprocess.run(cmd, capture_output=True, text=True)
        if result.returncode == 0:
            print(f"[+] ISO created successfully: {ISO_OUT}")
            return True
        else:
            print(f"[!] xorriso error: {result.stderr}")
            return False
    except Exception as e:
        print(f"[!] xorriso failed: {e}")
        return False

def provide_instructions():
    """Provide instructions for building on different platforms."""
    print("\n" + "="*60)
    print("BUILD INSTRUCTIONS FOR BOOTABLE ISO")
    print("="*60)
    print("\nOption 1: Use WSL (Windows Subsystem for Linux)")
    print("  1. Install WSL: wsl --install")
    print("  2. In WSL, install tools: sudo apt-get install grub-pc-bin xorriso gcc-multilib")
    print("  3. Run: python3 create_bootable_iso.py")
    print("\nOption 2: Use Linux machine or VM")
    print("  1. Install: sudo apt-get install grub-pc-bin xorriso gcc-multilib")
    print("  2. Run: make iso")
    print("  3. Or: python3 create_bootable_iso.py")
    print("\nOption 3: Cross-compile on Windows")
    print("  1. Install MinGW-w64 or MSYS2")
    print("  2. Install GRUB for Windows")
    print("  3. Adjust paths in build scripts")
    print("\n" + "="*60)

def main():
    import argparse
    parser = argparse.ArgumentParser(description="Create bootable GRUB ISO")
    parser.add_argument("--clean", action="store_true")
    parser.add_argument("--instructions", action="store_true")
    args = parser.parse_args()
    
    if args.instructions:
        provide_instructions()
        return
    
    if args.clean:
        shutil.rmtree(ISO_DIR, ignore_errors=True)
        if ISO_OUT.exists():
            ISO_OUT.unlink()
        print("[+] Clean complete")
        return
    
    if not create_iso_structure():
        print("[!] Failed to create ISO structure")
        sys.exit(1)
    
    # Try different build methods
    success = False
    if build_with_grub_mkrescue():
        success = True
    elif build_with_xorriso():
        success = True
    else:
        print("\n[!] Could not build ISO automatically.")
        provide_instructions()
        sys.exit(1)
    
    if success:
        print(f"\n[+] SUCCESS! Bootable ISO created: {ISO_OUT}")
        print(f"[*] File size: {ISO_OUT.stat().st_size / (1024*1024):.2f} MB")
        print(f"\n[*] To test in VirtualBox:")
        print(f"    1. Create new VM (Type: Other, Version: Other/Unknown)")
        print(f"    2. Settings > Storage > Add optical drive")
        print(f"    3. Select: {ISO_OUT}")
        print(f"    4. Start VM")
        print(f"\n[*] To test in QEMU:")
        print(f"    qemu-system-i386 -cdrom {ISO_OUT} -m 512")

if __name__ == "__main__":
    main()


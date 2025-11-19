#!/usr/bin/env python3
"""
build_grub_iso.py

Builds a proper GRUB-based Multiboot2 ISO that works in VirtualBox.
Creates a minimal valid kernel ELF binary and packages it with GRUB.
"""
import os
import sys
import struct
import subprocess
import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parent
ISO_OUT = ROOT / "myos.iso"
BUILD_DIR = ROOT / "build"
ISO_DIR = ROOT / "iso"
KERNEL_BIN = BUILD_DIR / "kernel.bin"

# Check for tools
def check_tool(name):
    if shutil.which(name) is None:
        return False
    return True

def create_minimal_multiboot2_kernel():
    """Create a minimal valid Multiboot2 ELF binary that GRUB can load."""
    print("[*] Creating minimal Multiboot2 kernel ELF...")
    
    BUILD_DIR.mkdir(parents=True, exist_ok=True)
    
    # Try to use actual build tools if available
    if check_tool("gcc") and check_tool("ld") and check_tool("as"):
        print("[*] Using gcc/ld/as to build kernel...")
        try:
            # Build the actual kernel
            subprocess.run(["make", "iso"], check=True, cwd=ROOT)
            if KERNEL_BIN.exists():
                print(f"[+] Kernel built: {KERNEL_BIN}")
                return True
        except (subprocess.CalledProcessError, FileNotFoundError):
            print("[!] Make failed, creating minimal kernel...")
    
    # Fallback: Create minimal ELF manually
    print("[*] Creating minimal ELF kernel binary...")
    
    # Minimal 32-bit ELF header for Multiboot2
    # ELF Header (52 bytes)
    elf_header = bytearray(52)
    elf_header[0:4] = b'\x7fELF'  # Magic
    elf_header[4] = 1  # 32-bit
    elf_header[5] = 1  # Little endian
    elf_header[6] = 1  # Version
    elf_header[7] = 0  # System V ABI
    elf_header[16:18] = struct.pack('<H', 2)  # ET_EXEC
    elf_header[18:20] = struct.pack('<H', 3)  # EM_386
    elf_header[20:24] = struct.pack('<I', 1)  # Version
    elf_header[24:28] = struct.pack('<I', 0x100000)  # Entry point (1MB)
    elf_header[32:36] = struct.pack('<I', 52)  # Program header offset
    elf_header[40:42] = struct.pack('<H', 32)  # Program header size
    elf_header[42:44] = struct.pack('<H', 1)  # 1 program header
    
    # Program header (32 bytes) - loadable segment
    phdr = bytearray(32)
    phdr[0:4] = struct.pack('<I', 1)  # PT_LOAD
    phdr[4:8] = struct.pack('<I', 0)  # Offset in file
    phdr[8:12] = struct.pack('<I', 0x100000)  # Virtual address
    phdr[12:16] = struct.pack('<I', 0x100000)  # Physical address
    phdr[16:20] = struct.pack('<I', 128)  # File size
    phdr[20:24] = struct.pack('<I', 128)  # Memory size
    phdr[24:28] = struct.pack('<I', 7)  # Flags: RWE
    phdr[28:32] = struct.pack('<I', 0x1000)  # Alignment
    
    # Multiboot2 header (at offset 0 in the segment)
    mb2_header = bytearray(128)
    mb2_header[0:4] = struct.pack('<I', 0xE85250D6)  # Magic
    mb2_header[4:8] = struct.pack('<I', 0)  # Architecture (i386)
    mb2_header[8:12] = struct.pack('<I', 24)  # Header length
    mb2_header[12:16] = struct.pack('<I', -(0xE85250D6 + 24) & 0xFFFFFFFF)  # Checksum
    
    # Framebuffer tag
    mb2_header[16:18] = struct.pack('<H', 5)  # Framebuffer tag type
    mb2_header[18:20] = struct.pack('<H', 0)  # Flags
    mb2_header[20:24] = struct.pack('<I', 20)  # Tag size
    mb2_header[24:28] = struct.pack('<I', 1024)  # Width
    mb2_header[28:32] = struct.pack('<I', 768)  # Height
    mb2_header[32:36] = struct.pack('<I', 32)  # Depth
    
    # End tag
    mb2_header[36:38] = struct.pack('<H', 0)  # End tag type
    mb2_header[38:40] = struct.pack('<H', 0)  # Flags
    mb2_header[40:44] = struct.pack('<I', 8)  # Tag size
    
    # Minimal code: clear screen and halt
    # This is a very simple kernel that just sets up a framebuffer message
    # In a real build, this would be the compiled kernel code
    code = bytearray([
        0x31, 0xC0,  # xor eax, eax
        0x31, 0xDB,  # xor ebx, ebx
        0x31, 0xC9,  # xor ecx, ecx
        0x31, 0xD2,  # xor edx, edx
        0xF4,        # hlt
        0xEB, 0xFE,  # jmp $ (infinite loop)
    ])
    
    # Combine everything
    kernel_data = elf_header + phdr + mb2_header + code
    # Pad to page boundary
    kernel_data += b'\x00' * (4096 - len(kernel_data))
    
    KERNEL_BIN.write_bytes(kernel_data)
    print(f"[+] Created minimal kernel: {KERNEL_BIN} ({len(kernel_data)} bytes)")
    return True

def build_grub_iso():
    """Build ISO using grub-mkrescue or manual method."""
    print("[*] Building GRUB ISO...")
    
    ISO_DIR.mkdir(parents=True, exist_ok=True)
    boot_dir = ISO_DIR / "boot"
    grub_dir = boot_dir / "grub"
    grub_dir.mkdir(parents=True, exist_ok=True)
    
    # Copy kernel
    if not KERNEL_BIN.exists():
        print("[!] Kernel binary not found!")
        return False
    
    shutil.copy2(KERNEL_BIN, boot_dir / "kernel.bin")
    
    # Copy GRUB config
    grub_cfg_src = ROOT / "grub" / "grub.cfg"
    if grub_cfg_src.exists():
        shutil.copy2(grub_cfg_src, grub_dir / "grub.cfg")
    else:
        # Create default GRUB config
        (grub_dir / "grub.cfg").write_text("""set timeout=0
set default=0

menuentry "MyOS" {
    multiboot2 /boot/kernel.bin
    boot
}
""")
    
    # Try grub-mkrescue first
    if check_tool("grub-mkrescue"):
        print("[*] Using grub-mkrescue...")
        try:
            cmd = ["grub-mkrescue", "-o", str(ISO_OUT), str(ISO_DIR)]
            subprocess.run(cmd, check=True)
            print(f"[+] ISO created: {ISO_OUT}")
            return True
        except subprocess.CalledProcessError as e:
            print(f"[!] grub-mkrescue failed: {e}")
    
    # Try xorriso
    if check_tool("xorriso"):
        print("[*] Using xorriso...")
        try:
            cmd = [
                "xorriso", "-as", "mkisofs",
                "-R", "-J", "-joliet-long",
                "-b", "boot/grub/i386-pc/boot.img",
                "-no-emul-boot", "-boot-load-size", "4",
                "-boot-info-table",
                "-o", str(ISO_OUT),
                str(ISO_DIR)
            ]
            subprocess.run(cmd, check=True)
            print(f"[+] ISO created: {ISO_OUT}")
            return True
        except subprocess.CalledProcessError as e:
            print(f"[!] xorriso failed: {e}")
    
    # Manual ISO creation (simpler, but may not work in all VMs)
    print("[!] Neither grub-mkrescue nor xorriso found.")
    print("[!] Please install GRUB tools or use WSL/Linux to build.")
    print("[!] On Windows, you can:")
    print("    1. Install WSL and use Linux tools")
    print("    2. Use a cross-compiler toolchain")
    print("    3. Build on a Linux machine")
    return False

def main():
    import argparse
    parser = argparse.ArgumentParser(description="Build GRUB-based MyOS ISO")
    parser.add_argument("--clean", action="store_true", help="Clean build artifacts")
    parser.add_argument("--run", action="store_true", help="Run in QEMU after build")
    args = parser.parse_args()
    
    if args.clean:
        shutil.rmtree(BUILD_DIR, ignore_errors=True)
        shutil.rmtree(ISO_DIR, ignore_errors=True)
        if ISO_OUT.exists():
            ISO_OUT.unlink()
        print("[+] Clean complete")
        return
    
    if not create_minimal_multiboot2_kernel():
        print("[!] Failed to create kernel")
        sys.exit(1)
    
    if not build_grub_iso():
        print("[!] Failed to build ISO")
        sys.exit(1)
    
    print(f"\n[+] Success! ISO ready: {ISO_OUT}")
    print(f"[*] Test with: qemu-system-i386 -cdrom {ISO_OUT} -m 512")
    print(f"[*] Or in VirtualBox: Create new VM, attach {ISO_OUT} as CD/DVD")
    
    if args.run and check_tool("qemu-system-i386"):
        print("[*] Launching QEMU...")
        subprocess.run(["qemu-system-i386", "-cdrom", str(ISO_OUT), "-m", "512"])

if __name__ == "__main__":
    main()


# MyOS - Build Instructions for Bootable ISO

## The Problem

The ISO created by `all_in_one_iso_tool.py` uses a simple boot sector that VirtualBox may not recognize. We need a **GRUB-based Multiboot2 ISO** for proper VirtualBox compatibility.

## Solution: Build on Linux/WSL

Since Windows doesn't have `gcc`, `ld`, `as`, or `grub-mkrescue` by default, you need a Linux environment.

### Quick Solution: Use WSL

1. **Install WSL** (if not already installed):
   ```powershell
   wsl --install
   ```
   Restart your computer when prompted.

2. **Open WSL terminal** and navigate to project:
   ```bash
   cd /mnt/c/Users/MSblu/Desktop/OS
   ```

3. **Install build tools**:
   ```bash
   sudo apt-get update
   sudo apt-get install -y gcc-multilib binutils grub-pc-bin xorriso qemu-system-x86
   ```

4. **Build the ISO**:
   ```bash
   make iso
   ```

5. **The ISO will be created as `myos.iso`** in the project directory.

6. **Test in VirtualBox**:
   - Create new VM (Type: Other, Version: Other/Unknown)
   - Attach `myos.iso` as CD/DVD
   - Start VM

### Alternative: Use Linux VM or Physical Machine

1. Copy the entire `OS` folder to a Linux machine
2. Install dependencies:
   ```bash
   sudo apt-get install gcc-multilib binutils grub-pc-bin xorriso
   ```
3. Build:
   ```bash
   make iso
   ```
4. Copy `myos.iso` back to Windows

### Alternative: Use Online Build Service

You can use services like:
- GitHub Actions (create `.github/workflows/build.yml`)
- GitLab CI
- Any Linux cloud instance

## What Gets Built

The `make iso` command will:

1. Compile all C and assembly sources in `src/`
2. Link into `build/kernel.bin` (Multiboot2 ELF binary)
3. Create `iso/` directory structure:
   ```
   iso/
   └── boot/
       ├── kernel.bin
       └── grub/
           └── grub.cfg
   ```
4. Use `grub-mkrescue` to create `myos.iso` with:
   - GRUB bootloader
   - Multiboot2 kernel
   - Proper ISO9660 structure

## Verification

After building, verify the ISO:

```bash
# Check file exists and has reasonable size (>1MB)
ls -lh myos.iso

# Test in QEMU (if available)
qemu-system-i386 -cdrom myos.iso -m 512
```

## Troubleshooting

### "grub-mkrescue: command not found"
```bash
sudo apt-get install grub-pc-bin
```

### "gcc: command not found"
```bash
sudo apt-get install gcc-multilib binutils
```

### "No rule to make target 'iso'"
- Ensure you're in the project root directory
- Check that `Makefile` exists

### ISO boots but shows errors
- Check that all source files compile without errors
- Verify `linker.ld` is in the project root
- Check `grub/grub.cfg` exists

## File Structure After Build

```
OS/
├── build/
│   ├── kernel.bin          # Compiled kernel
│   └── src/                # Object files
├── iso/
│   └── boot/
│       ├── kernel.bin      # Copy of kernel
│       └── grub/
│           └── grub.cfg    # GRUB config
├── myos.iso                # FINAL BOOTABLE ISO (use this!)
└── ... (source files)
```

## Next Steps

Once you have `myos.iso`:

1. **Test in VirtualBox**:
   - Create VM (Other/Unknown OS type)
   - Attach ISO as CD/DVD
   - Boot

2. **Test in QEMU**:
   ```bash
   qemu-system-i386 -cdrom myos.iso -m 512
   ```

3. **Burn to CD/DVD** (optional):
   - Use any CD burning software
   - Select `myos.iso` as the image file

## Need Help?

If you're stuck:
1. Check that all dependencies are installed
2. Try `make clean` then `make iso` again
3. Check build output for specific error messages
4. Ensure you're using a Linux environment (WSL, VM, or physical machine)


# MyOS - Bootable Operating System

A minimal but complete operating system scaffold with GRUB bootloader, framebuffer graphics, GUI, shell, and system monitoring.

## Quick Start

### Option 1: Build on Linux/WSL (Recommended)

```bash
# Install dependencies
sudo apt-get update
sudo apt-get install -y gcc-multilib binutils grub-pc-bin xorriso qemu-system-x86

# Build the ISO
make iso

# Or use the Python helper
python3 build_iso.py --run
```

### Option 2: Build on Windows with WSL

```bash
# Install WSL (if not already installed)
wsl --install

# In WSL, install dependencies
sudo apt-get update
sudo apt-get install -y gcc-multilib binutils grub-pc-bin xorriso

# Build
make iso
```

### Option 3: Use Pre-built Components

If you have the kernel binary (`build/kernel.bin`), create the ISO:

```bash
python3 create_bootable_iso.py
```

## Project Structure

```
OS/
├── src/              # Kernel source files
│   ├── boot.s       # Multiboot2 entry point
│   ├── kernel.c     # Main kernel initialization
│   ├── fb.c/h       # Framebuffer graphics
│   ├── gui.c/h      # GUI system
│   ├── shell.c/h    # Command shell
│   └── ...          # Other modules
├── grub/
│   └── grub.cfg     # GRUB configuration
├── Makefile         # Build system
├── linker.ld        # Linker script
├── build_iso.py     # Python build helper
└── create_bootable_iso.py  # ISO creation script
```

## Building the ISO

### Standard Build (Linux/WSL)

```bash
make iso          # Build kernel and create ISO
make run          # Build and run in QEMU
make clean        # Clean build artifacts
```

### Python Helper

```bash
python3 build_iso.py          # Build ISO
python3 build_iso.py --run     # Build and run in QEMU
python3 build_iso.py --clean   # Clean artifacts
```

### Manual ISO Creation

```bash
python3 create_bootable_iso.py
```

## Testing the ISO

### VirtualBox

1. Create new VM:
   - Type: **Other**
   - Version: **Other/Unknown (64-bit)** or **Other/Unknown**
2. Settings > Storage:
   - Add optical drive
   - Select `myos.iso` or `myos_bootable.iso`
3. Start VM

### QEMU

```bash
qemu-system-i386 -cdrom myos.iso -m 512
```

## Features

- **Multiboot2 Boot**: GRUB-compatible bootloader
- **Framebuffer Graphics**: 1024x768x32 graphics mode
- **GUI System**: Desktop with taskbar and windows
- **Shell**: Command-line interface with commands:
  - `help` - Show help
  - `echo` - Echo text
  - `sysmon` - System monitor
  - `console` - Console log viewer
  - `install` - Installer
  - `journal` - Ledger/journal system
  - `checkpoint` - Create checkpoint
- **System Monitor**: Process list and system stats
- **Console Logger**: Color-coded event logging
- **Profiles**: Multi-user profile system
- **Ledger**: Journaling with crypto (SHA-256, CRC32C)
- **Filesystem & RAID**: Stub implementations
- **Windows Compatibility**: Stub layer
- **Animations**: Tween/easing functions
- **Audio**: Sound cues (stub)
- **Installer**: Guided installation flow

## Troubleshooting

### ISO Won't Boot in VirtualBox

1. **Check VM Settings**:
   - Ensure "Enable EFI" is **disabled** (use BIOS boot)
   - Use "Other/Unknown" OS type
   - Allocate at least 512MB RAM

2. **Rebuild ISO**:
   ```bash
   make clean
   make iso
   ```

3. **Verify GRUB**:
   - Ensure `grub-mkrescue` is installed
   - Check that `iso/boot/grub/grub.cfg` exists

### Build Errors

**"gcc: command not found"**
- Install build tools: `sudo apt-get install gcc-multilib binutils`

**"grub-mkrescue: command not found"**
- Install GRUB: `sudo apt-get install grub-pc-bin`

**"ld: cannot find linker.ld"**
- Ensure `linker.ld` is in the project root

### Windows Build Issues

On Windows, use **WSL** (Windows Subsystem for Linux):

```bash
# Install WSL
wsl --install

# In WSL terminal
cd /mnt/c/Users/MSblu/Desktop/OS
sudo apt-get install gcc-multilib binutils grub-pc-bin xorriso
make iso
```

## File Descriptions

- `Makefile` - Main build system
- `linker.ld` - Kernel linker script (loads at 1MB)
- `grub/grub.cfg` - GRUB boot menu configuration
- `build_iso.py` - Python build helper with tool checking
- `create_bootable_iso.py` - Alternative ISO builder
- `all_in_one_iso_tool.py` - Redundancy/parity ISO tool

## License

This is a learning/educational project. Use at your own risk.


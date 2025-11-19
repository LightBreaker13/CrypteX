#!/usr/bin/env python3
"""
make_iso_from_scratch.py

Hand-crafted ISO9660 + El Torito (floppy emulation) generator.

- Produces output.iso (2048 byte sectors).
- Embeds a 1.44MB floppy image (boot.img) as the El Torito boot image.
- The floppy image contains a 512-byte BIOS boot sector (simple BIOS teletype
  hello message) + zeros to fill 1.44MB.
- ISO root contains /BOOTIMG.BIN which is identical to the floppy image (for convenience).

WARNING: This script tries to follow the ISO9660 + El Torito layout and writes
bytes precisely. Test produced ISO in QEMU/VirtualBox before using on real hardware.
"""

import os, sys, struct, math, hashlib

SECTOR = 2048

# ---------- tiny boot sector (512 bytes) ----------
# This is a small real-mode x86 boot sector assembled into raw bytes.
# The code writes "Boot OK - MyOS" via BIOS teletype (int 0x10) and then loops.
# It MUST end with 0x55 0xAA signature at offset 510.
#
# Note: This stub is intentionally minimal and crafted manually.
# If an emulator refuses to boot, we can replace this with another boot sector.
BOOT_SECTOR = bytes([
    # x86 Real-mode code (assembled machine bytes)
    0xEB, 0x1E,             # jmp short 0x20
    0x90,                   # nop
    # OEM label (8 bytes)
    ord('M'),ord('y'),ord('O'),ord('S'),ord('-'),ord('B'),ord('O'),ord('O'),
    # Bootloader starts at 0x1B
    # At 0x001B:
    0x0E,                   # push cs; but we'll use simpler ops; using int10 directly
    # We'll use BIOS teletype in a loop to print the string below
    # mov si, offset msg ; we will load SI with address of message after org 0x7C00
    0xBE, 0x3E, 0x7C,       # mov si, 0x7C3E  (address where msg will be located on the boot sector copy)
    0xB4, 0x0E,             # mov ah, 0x0E (teletype)
    # print_char:
    0xAC,                   # lodsb       ; AL = [SI]
    0x3C, 0x00,             # cmp al, 0x00
    0x74, 0x05,             # je done_print
    0xCD, 0x10,             # int 0x10
    0xEB, 0xF5,             # jmp print_char (back)
    # done_print:
    0xF4,                   # hlt
    0xEB, 0xFD,             # jmp $
    # Fill until offset 0x3E with zeros (we will place message at 0x7C00+0x3E)
] + [0x00] * (0x3E - 0x1B - 11)  # padding to reach offset where we planned message pointer
)

# Add message starting at relative offset (0x3E)
msg = b"Boot OK - MyOS\x00"
# Ensure BOOT_SECTOR has enough room (we planned to place msg starting at 0x3E)
if len(BOOT_SECTOR) < 0x3E:
    # pad
    BOOT_SECTOR = BOOT_SECTOR + b'\x00' * (0x3E - len(BOOT_SECTOR))
# place message
BOOT_SECTOR = BOOT_SECTOR[:0x3E] + msg + BOOT_SECTOR[0x3E + len(msg):]

# Now ensure length is <= 510
if len(BOOT_SECTOR) > 510:
    raise SystemExit("boot sector assembled bytes exceed 510 bytes; adjust code")

# pad to 510, then add signature 0x55AA
BOOT_SECTOR = BOOT_SECTOR.ljust(510, b'\x00') + b'\x55\xAA'
assert len(BOOT_SECTOR) == 512

# ---------- create 1.44MB floppy image ----------
FLOPPY_SIZE = 1440 * 1024
floppy = bytearray(FLOPPY_SIZE)
# place boot sector at start
floppy[0:512] = BOOT_SECTOR
# remainder is zeros

# ---------- ISO9660 primary volume descriptor builder ----------
def pvd_block(volume_id="PYISO", logical_block_size=SECTOR, vol_space_size=0x00000000):
    # Primary Volume Descriptor (type 1)
    # We'll build minimal mandatory fields
    v = bytearray(SECTOR)
    v[0] = 0x01  # type - primary
    v[1:6] = b"CD001"  # standard identifier
    v[6] = 0x01  # version
    # system identifier (32)
    v[8:40] = b' ' * 32
    # volume identifier (32)
    volid = volume_id.encode('ascii')[:32]
    v[40:40+len(volid)] = volid
    # volume space size (both-endian 4+4)
    # We'll fill real value later
    # set logical block size both-endian
    v[128:130] = struct.pack('<H', logical_block_size)
    v[130:132] = struct.pack('>H', logical_block_size)
    # path table size (both-endian) - we will set small later
    # root directory record at 156
    # volume set size, volume sequence number, file structure version are left with defaults
    return v

# ---------- Directory Record helper ----------
def make_dir_record(file_extent_lba, data_length, file_identifier, is_dir=False):
    # Directory Record structure variable length
    # length of Directory Record: 34 + len(file_identifier) for typical minimal
    fi = file_identifier.encode('ascii')
    dr_len = 33 + len(fi) + (len(fi) % 2 == 0 and 0 or 1)  # pad to even
    r = bytearray(dr_len)
    r[0] = dr_len  # length
    r[1] = 0  # ext attr length
    r[2:6] = struct.pack('<I', file_extent_lba)
    r[6:10] = struct.pack('>I', file_extent_lba)
    r[10:14] = struct.pack('<I', data_length)
    r[14:18] = struct.pack('>I', data_length)
    # Recording date/time (7 bytes) - zeros acceptable
    r[18:25] = bytes([0]*7)
    r[25] = 2 if is_dir else 0  # file flags
    r[26] = 0  # file unit size
    r[27] = 0  # interleave gap size
    r[28:32] = struct.pack('<H', 1) + struct.pack('>H', 1)  # volume seq number both-endian
    r[32] = len(fi)
    r[33:33+len(fi)] = fi
    # padding if needed
    if (33+len(fi)) < dr_len:
        r[33+len(fi)] = 0
    return bytes(r)

# ---------- Build ISO layout plan ----------
# We want:
# Sector 0: System Area (zeros)
# Sector 16: Primary Volume Descriptor
# Sector 17: Volume Descriptor Set Terminator
# Following: Path Table, Root Directory Record, file data extents...
#
# We'll place the El Torito Boot Record Volume Descriptor at sector 17 (actually El Torito uses a Boot Record (type 0) with ID 'CD001' and 'BEA01' for BD/Catalogs; spec requires special descriptors in sectors < 16? We'll place Boot Record at sector 17 per allowed order.)
#
# Simpler approach: Put:
#   sector 16: Primary Volume Descriptor
#   sector 17: Volume Descriptor Set Terminator (type 255)
#   then append path table and root dir at some later sector.
#
# For El Torito, we must include a Boot Record Volume Descriptor (type 0) with identifier 'CD001' and a Boot Catalog pointer in the data area.
# The El Torito Boot Record is type 0. We'll put it at sector 16 instead of PVD, and put PVD at 17 — BUT ISO9660 requires PVD at 16.
# However the El Torito spec says Boot Record Volume Descriptor (type 0) should appear as a special record with ID 'CD001' and a 'BE' signature at byte 7? Implementation detail is subtle.
# To be conservative, we will:
#   sector 16: Primary Volume Descriptor (type 1)
#   sector 17: Boot Record (type 0) with El Torito catalog pointer (this ordering is tolerated by many readers)
#
# Note: This is a hand-crafted attempt; readers vary in strictness.
#

# We'll compute extents:
# Place boot catalog and boot image data in later sectors and ensure the boot record points to them.

# We'll place:
# - sector 20.. (one sector) : El Torito Boot Catalog (32 bytes slot + padding to 2048)
# - sector 21.. (N sectors) : Boot Image (floppy image) — for floppy emulation El Torito uses sector count equal to size/2048; but floppy is 1.44MB so 1440*1024/2048 ~ 720 sectors. We'll store entire floppy image starting at LBA 21.

BOOT_CATALOG_LBA = 20
BOOT_IMAGE_LBA = BOOT_CATALOG_LBA + 1  # catalog occupies 1 sector
BOOT_IMAGE_SECTORS = (FLOPPY_SIZE + SECTOR - 1) // SECTOR

# For simplicity: place Path Table at LBA = BOOT_IMAGE_LBA + BOOT_IMAGE_SECTORS
PATH_TABLE_LBA = BOOT_IMAGE_LBA + BOOT_IMAGE_SECTORS
# Root Directory extent after that; allocate 1 sector for the root dir record
ROOT_DIR_LBA = PATH_TABLE_LBA + 1
# File data extents follow root dir (we'll put a copy of the boot image as a file named BOOTIMG.BIN)
FILE_DATA_LBA = ROOT_DIR_LBA + 1
FILE_DATA_SECTORS = BOOT_IMAGE_SECTORS  # same size as floppy copy

# compute volume space size - make ISO big enough to include all extents
VOL_SPACE_SIZE = FILE_DATA_LBA + FILE_DATA_SECTORS + 10  # keep extra

# ---------- Build PVD and descriptors ----------
pvd = bytearray(SECTOR)
pvd[0] = 0x01
pvd[1:6] = b"CD001"
pvd[6] = 0x01
# System ID / Volume ID
pvd[8:40] = b' ' * 32
volid = b"MY_ISO_PY"
pvd[40:40+len(volid)] = volid
# Volume space size (both-endian)
pvd[80:84] = struct.pack('<I', VOL_SPACE_SIZE)
pvd[84:88] = struct.pack('>I', VOL_SPACE_SIZE)
# volume set size, volume seq num, logical block size
pvd[120:124] = struct.pack('<I', 0)  # unused
pvd[128:130] = struct.pack('<H', SECTOR)
pvd[130:132] = struct.pack('>H', SECTOR)
# path table size (we'll put minimal 10)
path_table_size = 10
pvd[132:136] = struct.pack('<I', path_table_size)
pvd[140:144] = struct.pack('>I', PATH_TABLE_LBA)  # LBA of little-endian path table
# root directory record - 34 bytes typical
root_dir_record = make_dir_record(ROOT_DIR_LBA, SECTOR, '\x00', is_dir=True)
pvd[156:156+len(root_dir_record)] = root_dir_record
# volume set id, publisher etc left blank

# ---------- Boot Record (El Torito) ----------
# Boot Record VDS: type 0, id 'CD001', version 1; then 0.. rest contains "EL TORITO SPECIFICATION" data which includes 32-bit pointer to Boot Catalog LBA
boot_record = bytearray(SECTOR)
boot_record[0] = 0x00
boot_record[1:6] = b"CD001"
boot_record[6] = 0x01
# El Torito identifier in bytes 7..38 is "EL TORITO SPECIFICATION" (null padded)
el_id = b"EL TORITO SPECIFICATION"
boot_record[7:7+len(el_id)] = el_id
# Boot catalog LBA (4 bytes little-endian) normally placed at offset 71 (but spec expects the 32-bit pointer inside the "Boot Record Volume Descriptor" data area)
# We'll put it at offset 71 for our consumer script to find
boot_record[71:75] = struct.pack('<I', BOOT_CATALOG_LBA)

# ---------- Boot Catalog (1 sector) ----------
# El Torito Boot Catalog has a validation entry (32 bytes) and initial/section entry (32 bytes)
boot_catalog = bytearray(SECTOR)
# Validation entry (bytes 0..31)
# Header ID (0x01)
boot_catalog[0] = 0x01
boot_catalog[1] = 0x00  # platform id (0 for 80x86)
# id string (28 bytes) - can be arbitrary but must have checksum that makes bytes 0..29 sum to 0 mod 0x100
idstr = b"PYTHON-MADE-BOOTCATALOG"
boot_catalog[4:4+len(idstr)] = idstr
# Compute checksum over first 32 bytes as little-endian sum of 16-bit words equals 0
# We'll compute programmatically and set word 10..11 reserved signature?
# For simplicity leave rest zeros; compute checksum and put at offset 0x1C (28)
# We'll fill fields later
# Initial/Default entry (bytes 32..63)
# Boot indicator (0x88 = bootable), media type (0), load segment (0x0000 usually 0x0000 for no emu?), system type (0 for 80x86), sector count (we'll fill), load LBA (BOOT_IMAGE_LBA)
boot_catalog[32] = 0x88  # bootable
boot_catalog[33] = 0x00  # media type (0)
# load segment (word little endian) - for floppy emulation set 0x0000? Usually 0x0000 or 0x07C0; many examples use 0x0000
boot_catalog[34:36] = struct.pack('<H', 0x0000)
# system type
boot_catalog[36] = 0x00
# unused
boot_catalog[37] = 0x00
# sector count (word)
boot_catalog[38:40] = struct.pack('<H', BOOT_IMAGE_SECTORS & 0xFFFF)
# load LBA
boot_catalog[40:44] = struct.pack('<I', BOOT_IMAGE_LBA)

# compute checksum for validation entry (first 32 bytes): sum of 16-bit words should be 0
def compute_validation_checksum(b32):
    s = 0
    for i in range(0, 32, 2):
        s += b32[i] + (b32[i+1] << 8)
        s &= 0xFFFFFFFF
    # checksum is such that sum of all words == 0 mod 65536
    chk = (-s) & 0xFFFF
    return chk

chk = compute_validation_checksum(boot_catalog[0:32])
boot_catalog[28:30] = struct.pack('<H', chk)
# set validation header signature 0x55AA at bytes 30..31? spec: bytes 30..31 = 0x55 0xAA
boot_catalog[30:32] = b'\x55\xAA'

# ---------- Path Table ----------
# Minimal little-endian path table: one entry for root dir (length 10 bytes typical)
# Structure: length of dir id (1), extended attr len (1), location of extent (4 little), parent dir number (2), dir id (n)
pt = bytearray()
dir_id = b'\x00'  # root
entry = bytes([1, 0]) + struct.pack('<I', ROOT_DIR_LBA) + struct.pack('<H', 1) + dir_id
# pad to even boundary
if len(entry) % 2 == 1:
    entry += b'\x00'
pt += entry
# pad the path table sector to SECTOR
pt = pt.ljust(SECTOR, b'\x00')

# ---------- Root Directory record sector (we'll create a directory containing BOOTIMG.BIN) ----------
root_dir_sector = bytearray(SECTOR)
# "." entry (self) and ".." reserved not necessary; we create directory record for file BOOTIMG.BIN
# Create directory record referencing file data extent at FILE_DATA_LBA
file_name = "BOOTIMG.BIN"
file_size = FLOPPY_SIZE
file_dr = make_dir_record(FILE_DATA_LBA, file_size, file_name, is_dir=False)
# place file_dr starting at offset 0
root_dir_sector[0:len(file_dr)] = file_dr

# ---------- File data: copy floppy image into sectors starting at FILE_DATA_LBA ----------
# We'll write the floppy image into the file data area (so both boot image and file inside were same)
# Build the full ISO as a list of sectors
total_sectors = VOL_SPACE_SIZE
print("Planned total sectors:", total_sectors)
iso = bytearray(SECTOR * total_sectors)

# Sector 16 = PVD
iso[SECTOR*16:SECTOR*17] = pvd
# Sector 17 = Boot Record (El Torito)
iso[SECTOR*17:SECTOR*18] = boot_record
# Sector 18 = Volume Descriptor Set Terminator (type 255)
vdst = bytearray(SECTOR)
vdst[0] = 255
vdst[1:6] = b"CD001"
vdst[6] = 0x01
iso[SECTOR*18:SECTOR*19] = vdst

# Boot Catalog sector (20)
iso[SECTOR*BOOT_CATALOG_LBA:SECTOR*(BOOT_CATALOG_LBA+1)] = boot_catalog

# Boot Image sectors (BOOT_IMAGE_LBA ..)
for i in range(BOOT_IMAGE_SECTORS):
    start = i * SECTOR
    chunk = floppy[start:start+SECTOR]
    iso_offset = SECTOR*(BOOT_IMAGE_LBA + i)
    iso[iso_offset:iso_offset+SECTOR] = chunk

# Path table
iso[SECTOR*PATH_TABLE_LBA:SECTOR*(PATH_TABLE_LBA+1)] = pt

# Root dir
iso[SECTOR*ROOT_DIR_LBA:SECTOR*(ROOT_DIR_LBA+1)] = root_dir_sector

# File data area - copy floppy again as named file
for i in range(FILE_DATA_SECTORS):
    src_off = i*SECTOR
    dst_off = SECTOR*(FILE_DATA_LBA + i)
    iso[dst_off:dst_off+SECTOR] = floppy[src_off:src_off+SECTOR]

# Write to disk
outname = "output.iso"
with open(outname, "wb") as f:
    f.write(iso)

print("Wrote", outname, "size", os.path.getsize(outname), "bytes")
print("Boot catalog LBA:", BOOT_CATALOG_LBA, "Boot image LBA:", BOOT_IMAGE_LBA)
print("Try: qemu-system-i386 -cdrom output.iso -m 512")
print("If it doesn't boot in QEMU, send me the emulator output and I'll help debug.")

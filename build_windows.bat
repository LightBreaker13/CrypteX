@echo off
REM Windows build script for MyOS
REM Automatically uses WSL if available, otherwise provides instructions

echo [*] MyOS Build Script for Windows
echo.

REM Check for WSL
where wsl >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo [+] WSL detected, using WSL to build...
    echo.
    wsl bash -c "cd /mnt/c/Users/MSblu/Desktop/OS && sudo apt-get update && sudo apt-get install -y gcc-multilib binutils grub-pc-bin xorriso 2>/dev/null || true && make iso"
    if %ERRORLEVEL% EQU 0 (
        echo.
        echo [+] Build successful! ISO created: myos.iso
        echo [*] You can now use this ISO in VirtualBox
        goto :end
    )
)

REM Check for native tools
where gcc >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo [+] Native GCC found, attempting build...
    make iso
    if %ERRORLEVEL% EQU 0 (
        echo.
        echo [+] Build successful! ISO created: myos.iso
        goto :end
    )
)

echo.
echo [!] Build tools not found on Windows
echo.
echo Please use one of these options:
echo.
echo Option 1: Install WSL
echo   1. Run: wsl --install
echo   2. Restart computer
echo   3. Run this script again
echo.
echo Option 2: Use Linux VM
echo   1. Copy this folder to a Linux VM
echo   2. Run: sudo apt-get install gcc-multilib binutils grub-pc-bin xorriso
echo   3. Run: make iso
echo.
echo Option 3: Use pre-built ISO
echo   If you have a Linux machine, build there and copy myos.iso back
echo.

:end
pause


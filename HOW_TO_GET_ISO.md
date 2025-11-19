# How to Get Your Bootable ISO File

Since Windows doesn't have the build tools (`gcc`, `grub-mkrescue`) by default, here are **3 ways** to get your bootable `myos.iso`:

## 🚀 Option 1: Use WSL (Easiest for Windows)

1. **Install WSL** (one-time setup):
   ```powershell
   wsl --install
   ```
   Restart when prompted.

2. **Open WSL terminal** and run:
   ```bash
   cd /mnt/c/Users/MSblu/Desktop/OS
   sudo apt-get update
   sudo apt-get install -y gcc-multilib binutils grub-pc-bin xorriso
   make iso
   ```

3. **Done!** Your `myos.iso` is ready in the `OS` folder.

## ☁️ Option 2: Use GitHub Actions (Automatic)

1. **Create a GitHub repository** and push this code
2. **GitHub will automatically build** the ISO using the workflow in `.github/workflows/build.yml`
3. **Download the ISO** from the Actions tab:
   - Go to your repo → Actions
   - Click on the latest workflow run
   - Download the `myos-iso` artifact

**No local tools needed!**

## 💻 Option 3: Use a Linux Machine/VM

1. **Copy the `OS` folder** to any Linux machine (or VM)
2. **Install tools**:
   ```bash
   sudo apt-get install gcc-multilib binutils grub-pc-bin xorriso
   ```
3. **Build**:
   ```bash
   make iso
   ```
4. **Copy `myos.iso`** back to Windows

## ✅ Verify Your ISO

After getting `myos.iso`:

1. **Check it exists**: Should be in the `OS` folder, size ~1-5 MB
2. **Test in VirtualBox**:
   - Create VM (Type: Other, Version: Other/Unknown)
   - Attach `myos.iso` as CD/DVD
   - Boot!

## 📋 Quick Reference

| Method | Time | Difficulty | Tools Needed |
|--------|------|------------|--------------|
| WSL | 10 min | Easy | WSL installed |
| GitHub Actions | 5 min | Very Easy | GitHub account |
| Linux VM | 15 min | Medium | Linux VM |

## 🆘 Still Stuck?

- See `BUILD_INSTRUCTIONS.md` for detailed steps
- See `README.md` for full documentation
- Check that `myos.iso` exists and is >1 MB in size

---

**Recommended**: Use **Option 1 (WSL)** for fastest local build, or **Option 2 (GitHub Actions)** if you don't want to install anything locally.


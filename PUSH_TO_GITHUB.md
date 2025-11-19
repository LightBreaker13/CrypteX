# Push to GitHub and Build ISO Automatically

This guide will help you push your code to GitHub and use GitHub Actions to automatically build the bootable ISO.

## Step 1: Install Git (if not installed)

### Option A: Download Git for Windows
1. Go to: https://git-scm.com/download/win
2. Download and install Git for Windows
3. Restart your terminal/PowerShell

### Option B: Install via winget
```powershell
winget install Git.Git
```

## Step 2: Create GitHub Repository

1. Go to https://github.com/new
2. Create a new repository:
   - Name: `myos` (or any name you like)
   - Description: "Minimal operating system with GRUB bootloader"
   - Choose **Public** or **Private**
   - **DO NOT** initialize with README, .gitignore, or license (we already have these)
3. Click "Create repository"

## Step 3: Initialize Git and Push

Open PowerShell in the `OS` folder and run:

```powershell
# Initialize git repository
git init

# Add all files
git add .

# Create initial commit
git commit -m "Initial commit: MyOS kernel with GRUB bootloader"

# Add GitHub remote (replace YOUR_USERNAME and YOUR_REPO_NAME)
git remote add origin https://github.com/YOUR_USERNAME/YOUR_REPO_NAME.git

# Push to GitHub
git branch -M main
git push -u origin main
```

**Replace `YOUR_USERNAME` and `YOUR_REPO_NAME` with your actual GitHub username and repository name!**

## Step 4: Wait for GitHub Actions

1. Go to your repository on GitHub
2. Click the **Actions** tab
3. You should see a workflow run starting
4. Wait 2-5 minutes for it to complete
5. Once done, click on the workflow run
6. Scroll down to **Artifacts**
7. Download **myos-iso** - this is your bootable ISO!

## Alternative: Use GitHub CLI (gh)

If you have GitHub CLI installed:

```powershell
# Install GitHub CLI (if needed)
winget install GitHub.cli

# Login
gh auth login

# Create repo and push
gh repo create myos --public --source=. --remote=origin --push
```

## Troubleshooting

### "git: command not found"
- Install Git for Windows (see Step 1)
- Restart your terminal

### "Authentication failed"
- Use a Personal Access Token instead of password
- Go to: https://github.com/settings/tokens
- Generate new token (classic) with `repo` permissions
- Use token as password when pushing

### "Workflow not running"
- Check that `.github/workflows/build.yml` exists
- Ensure you pushed to `main` or `master` branch
- Check Actions tab for any errors

### "No artifacts found"
- Wait for workflow to complete (check the green checkmark)
- Artifacts are only available after successful build
- Check workflow logs for build errors

## Quick Copy-Paste Commands

After creating your GitHub repo, run these (replace placeholders):

```powershell
cd C:\Users\MSblu\Desktop\OS
git init
git add .
git commit -m "Initial commit: MyOS kernel"
git remote add origin https://github.com/YOUR_USERNAME/YOUR_REPO_NAME.git
git branch -M main
git push -u origin main
```

Then go to: `https://github.com/YOUR_USERNAME/YOUR_REPO_NAME/actions`


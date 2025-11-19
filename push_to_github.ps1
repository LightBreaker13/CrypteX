# PowerShell script to push MyOS to GitHub and trigger Actions build

Write-Host "=== MyOS GitHub Push Script ===" -ForegroundColor Cyan
Write-Host ""

# Check if git is installed
$gitInstalled = Get-Command git -ErrorAction SilentlyContinue
if (-not $gitInstalled) {
    Write-Host "[!] Git is not installed!" -ForegroundColor Red
    Write-Host ""
    Write-Host "Please install Git first:" -ForegroundColor Yellow
    Write-Host "  1. Download from: https://git-scm.com/download/win" -ForegroundColor Yellow
    Write-Host "  2. Or run: winget install Git.Git" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "After installing, restart PowerShell and run this script again." -ForegroundColor Yellow
    exit 1
}

Write-Host "[+] Git found: $($gitInstalled.Version)" -ForegroundColor Green
Write-Host ""

# Check if already a git repo
if (Test-Path .git) {
    Write-Host "[+] Git repository already initialized" -ForegroundColor Green
} else {
    Write-Host "[*] Initializing git repository..." -ForegroundColor Yellow
    git init
    Write-Host "[+] Repository initialized" -ForegroundColor Green
}

# Check if files are staged
$status = git status --porcelain
if ($status) {
    Write-Host "[*] Adding files to git..." -ForegroundColor Yellow
    git add .
    Write-Host "[+] Files added" -ForegroundColor Green
} else {
    Write-Host "[+] All files already staged" -ForegroundColor Green
}

# Check if there are commits
$commits = git log --oneline -1 2>$null
if (-not $commits) {
    Write-Host "[*] Creating initial commit..." -ForegroundColor Yellow
    git commit -m "Initial commit: MyOS kernel with GRUB bootloader"
    Write-Host "[+] Initial commit created" -ForegroundColor Green
} else {
    Write-Host "[+] Repository already has commits" -ForegroundColor Green
}

Write-Host ""
Write-Host "=== Next Steps ===" -ForegroundColor Cyan
Write-Host ""
Write-Host "1. Create a GitHub repository:" -ForegroundColor Yellow
Write-Host "   Go to: https://github.com/new" -ForegroundColor White
Write-Host "   - Name it (e.g., 'myos')" -ForegroundColor White
Write-Host "   - Choose Public or Private" -ForegroundColor White
Write-Host "   - DO NOT initialize with README" -ForegroundColor White
Write-Host ""
Write-Host "2. After creating the repo, run these commands:" -ForegroundColor Yellow
Write-Host ""
Write-Host "   git remote add origin https://github.com/YOUR_USERNAME/YOUR_REPO_NAME.git" -ForegroundColor White
Write-Host "   git branch -M main" -ForegroundColor White
Write-Host "   git push -u origin main" -ForegroundColor White
Write-Host ""
Write-Host "   (Replace YOUR_USERNAME and YOUR_REPO_NAME with your actual values)" -ForegroundColor Gray
Write-Host ""
Write-Host "3. Go to the Actions tab on GitHub to see the build:" -ForegroundColor Yellow
Write-Host "   https://github.com/YOUR_USERNAME/YOUR_REPO_NAME/actions" -ForegroundColor White
Write-Host ""
Write-Host "4. Download the ISO from the Actions artifacts!" -ForegroundColor Yellow
Write-Host ""

# Check if GitHub CLI is available
$ghInstalled = Get-Command gh -ErrorAction SilentlyContinue
if ($ghInstalled) {
    Write-Host "[+] GitHub CLI (gh) detected!" -ForegroundColor Green
    Write-Host ""
    $useGh = Read-Host "Would you like to create the repo automatically with GitHub CLI? (y/n)"
    if ($useGh -eq "y" -or $useGh -eq "Y") {
        Write-Host "[*] Creating GitHub repository..." -ForegroundColor Yellow
        $repoName = Read-Host "Enter repository name (e.g., 'myos')"
        if ($repoName) {
            gh repo create $repoName --public --source=. --remote=origin --push
            if ($LASTEXITCODE -eq 0) {
                Write-Host "[+] Repository created and pushed!" -ForegroundColor Green
                Write-Host "[*] Check Actions: https://github.com/$($(gh api user --jq .login))/${repoName}/actions" -ForegroundColor Cyan
            }
        }
    }
}

Write-Host ""
Write-Host "For detailed instructions, see: PUSH_TO_GITHUB.md" -ForegroundColor Gray


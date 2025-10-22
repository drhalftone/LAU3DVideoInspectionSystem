# Button 5: Start Operations Action
# Purpose: Launch the main operational process or show setup instructions
# This script is executed when the user clicks Button 5

param(
    [Parameter(Mandatory=$true)]
    [string]$InstallPath
)

# Check if LAUProcessVideos is already running
$processName = "LAUProcessVideos"
$process = Get-Process -Name $processName -ErrorAction SilentlyContinue

if ($process) {
    Write-Host "=========================================="
    Write-Host "OPERATIONS - Already Running"
    Write-Host "=========================================="
    Write-Host ""
    Write-Host "LAUProcessVideos.exe is already running."
    Write-Host ""
    Write-Host "To configure automated startup via Task Scheduler:"
    Write-Host "1. Press Windows+R and type: taskschd.msc"
    Write-Host "2. Click 'Create Task' in the right panel"
    Write-Host "3. General tab:"
    Write-Host "   - Name: LAUProcessVideos"
    Write-Host "   - Check: 'Run with highest privileges'"
    Write-Host "4. Triggers tab:"
    Write-Host "   - New trigger → 'At startup'"
    Write-Host "5. Actions tab:"
    Write-Host "   - New action → Start a program"
    Write-Host "   - Program: $InstallPath\LAUProcessVideos.exe"
    Write-Host "6. Click OK to save"
    Write-Host ""
    Write-Host "=========================================="
    exit 0
}

# Launch the process monitoring tool
$processTool = Join-Path $InstallPath "LAUProcessVideos.exe"

if (Test-Path $processTool) {
    Write-Host "Launching operations process: $processTool"
    Start-Process $processTool
    exit 0
} else {
    Write-Host "Error: Process tool not found at $processTool"
    Write-Host ""
    Write-Host "Expected location: $processTool"
    Write-Host "Please verify the installation includes LAUProcessVideos.exe"
    exit 1
}

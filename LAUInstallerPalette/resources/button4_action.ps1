# Button 4: Capture Baseline Action
# Purpose: Launch background capture tool
# This script is executed when the user clicks Button 4

param(
    [Parameter(Mandatory=$true)]
    [string]$InstallPath
)

# Launch the background filter/capture tool
$captureTool = Join-Path $InstallPath "LAUBackgroundFilter.exe"

if (Test-Path $captureTool) {
    Write-Host "Launching background capture tool: $captureTool"
    Start-Process $captureTool
    exit 0
} else {
    Write-Host "Error: Background capture tool not found at $captureTool"
    Write-Host ""
    Write-Host "Expected location: $captureTool"
    Write-Host "Please verify the installation includes LAUBackgroundFilter.exe"
    exit 1
}

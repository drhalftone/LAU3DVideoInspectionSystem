# Button 3: Configure Devices Action
# Purpose: Launch camera labeling tool or provide configuration instructions
# This script is executed when the user clicks Button 3

param(
    [Parameter(Mandatory=$true)]
    [string]$InstallPath
)

# Launch the camera labeling/configuration tool
$configTool = Join-Path $InstallPath "LAULabelCameras.exe"

if (Test-Path $configTool) {
    Write-Host "Launching camera labeling tool: $configTool"
    Start-Process $configTool
    exit 0
} else {
    Write-Host "Error: Camera labeling tool not found at $configTool"
    Write-Host ""
    Write-Host "Expected location: $configTool"
    Write-Host "Please verify the installation includes LAULabelCameras.exe"
    exit 1
}

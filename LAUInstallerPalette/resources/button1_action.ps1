# Button 1: System Configuration Action
# Purpose: Launch the system configuration tool
# This script is executed when the user clicks Button 1

param(
    [Parameter(Mandatory=$true)]
    [string]$InstallPath
)

$configTool = Join-Path $InstallPath "LAURemoteToolsScheduler.exe"

if (Test-Path $configTool) {
    Write-Host "Launching configuration tool: $configTool"
    Start-Process $configTool
    exit 0
} else {
    Write-Host "Error: Configuration tool not found at $configTool"
    exit 1
}

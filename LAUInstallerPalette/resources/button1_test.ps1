# Button 1: Farm Configuration Test
# Purpose: Check if farmConfig.ini exists
# Exit Code: 0 = Ready (GREEN), 1 = Not Ready (RED)
# Mimics: LAURemoteToolsPalette::checkButton1Status()

param(
    [Parameter(Mandatory=$true)]
    [string]$InstallPath
)

# Check if systemConfig.ini exists
$configPath = Join-Path $InstallPath "systemConfig.ini"

if (Test-Path $configPath) {
    Write-Host "✓ System configuration found: $configPath"
    exit 0  # SUCCESS - Ready (GREEN)
} else {
    Write-Host "✗ System configuration missing: $configPath"
    exit 1  # FAIL - Not Ready (RED)
}

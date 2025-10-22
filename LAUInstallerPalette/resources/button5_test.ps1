# Button 5: System Calibration Test
# Purpose: Check if system calibration is ready to perform or already complete
# Exit Code: 0 = Ready (GREEN), 1 = Not Ready (RED)

param(
    [Parameter(Mandatory=$true)]
    [string]$InstallPath
)

# Helper function to read config value
function Get-ConfigValue {
    param($configPath, $key)
    if (Test-Path $configPath) {
        $line = Get-Content $configPath | Where-Object { $_ -match "^$key=" }
        if ($line) {
            return ($line -replace "^$key=", '').Trim()
        }
    }
    return $null
}

# Check if there are uncalibrated files (noCal*.tif)
$configPath = Join-Path $InstallPath "systemConfig.ini"
$tempPath = Get-ConfigValue $configPath "LocalTempPath"

if ($tempPath -and (Test-Path $tempPath)) {
    $noCalFiles = Get-ChildItem -Path $tempPath -Filter "noCal*.tif" -ErrorAction SilentlyContinue
    if ($noCalFiles) {
        Write-Host "✓ Uncalibrated files found - ready to calibrate"
        exit 0  # SUCCESS - files need calibration
    }
}

# Check if system is already calibrated (background.tif exists with valid transform)
# Simplified check - just verify background exists
$sharedPath = "C:\ProgramData\3DVideoInspectionTools"
$backgroundPath = Join-Path $sharedPath "background.tif"

if (Test-Path $backgroundPath) {
    Write-Host "✓ System appears calibrated (background exists)"
    exit 0  # SUCCESS - already calibrated
}

Write-Host "✗ No calibration files or calibrated background found"
exit 1  # FAIL - not ready

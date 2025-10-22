# Button 6: Advanced Calibration Action
# Purpose: Launch calibration tool or open file selection dialog for noCal files
# This script is executed when the user clicks Button 6

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

# Check for noCal files that need calibration
$configPath = Join-Path $InstallPath "systemConfig.ini"
$tempPath = Get-ConfigValue $configPath "LocalTempPath"

if ($tempPath -and (Test-Path $tempPath)) {
    $noCalFiles = Get-ChildItem -Path $tempPath -Filter "noCal*.tif" -ErrorAction SilentlyContinue

    if ($noCalFiles) {
        Write-Host "=========================================="
        Write-Host "CALIBRATION - noCal Files Found"
        Write-Host "=========================================="
        Write-Host ""
        Write-Host "Found $($noCalFiles.Count) uncalibrated file(s) in:"
        Write-Host "  $tempPath"
        Write-Host ""
        Write-Host "Please use the calibration tool to:"
        Write-Host "  1. Set bounding boxes"
        Write-Host "  2. Define transform matrices"
        Write-Host "  3. Generate lookup tables (LUTX files)"
        Write-Host ""
        Write-Host "Opening temporary folder for file selection..."
        Write-Host "=========================================="

        # Open the temp folder in Windows Explorer
        Start-Process "explorer.exe" -ArgumentList $tempPath
        exit 0
    }
}

# Launch the calibration tool
$calibrationTool = Join-Path $InstallPath "LAUJetrStandalone.exe"

if (Test-Path $calibrationTool) {
    Write-Host "Launching calibration tool: $calibrationTool"
    Start-Process $calibrationTool
    exit 0
} else {
    Write-Host "Error: Calibration tool not found at $calibrationTool"
    Write-Host ""
    Write-Host "Expected location: $calibrationTool"
    Write-Host "Please verify the installation includes LAUJetrStandalone.exe"
    exit 1
}

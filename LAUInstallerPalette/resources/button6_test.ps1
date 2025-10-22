# Button 6: Video Processing Test
# Purpose: Check if system is fully calibrated for video processing
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

# Check if system is fully calibrated
# Requires BOTH: background.tif with transform matrix AND .lutx lookup table files

$sharedPath = "C:\ProgramData\3DVideoInspectionTools"
$backgroundPath = Join-Path $sharedPath "background.tif"

# Check background.tif exists
if (-not (Test-Path $backgroundPath)) {
    Write-Host "✗ Background image missing: $backgroundPath"
    exit 1  # FAIL - no background
}

# Check for LUTX lookup table files
$configPath = Join-Path $InstallPath "systemConfig.ini"
$tempPath = Get-ConfigValue $configPath "LocalTempPath"

if (-not $tempPath) {
    Write-Host "✗ LocalTempPath not configured"
    exit 1  # FAIL - no temp path
}

if (-not (Test-Path $tempPath)) {
    Write-Host "✗ Temp path does not exist: $tempPath"
    exit 1  # FAIL - temp path missing
}

$lutxFiles = Get-ChildItem -Path $tempPath -Filter "*.lutx" -ErrorAction SilentlyContinue

if ($lutxFiles) {
    Write-Host "✓ System fully calibrated (background + $($lutxFiles.Count) LUTX files)"
    exit 0  # SUCCESS - fully calibrated
} else {
    Write-Host "✗ No LUTX lookup table files found in: $tempPath"
    exit 1  # FAIL - missing lookup tables
}

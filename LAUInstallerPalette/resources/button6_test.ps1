# Button 6 Status Check: Calibrate System
# Returns:
#   0 (GREEN) if system is fully calibrated (background.tif AND LUTX files exist)
#   1 (RED) if system needs calibration

param(
    [string]$SharedPath = "C:\Users\Public\Pictures",
    [string]$TempPath = ""
)

# If TempPath not provided, try to read from config
if ([string]::IsNullOrEmpty($TempPath)) {
    $configPath = "C:\LAU3DVideoInspectionTools\systemConfig.ini"
    if (Test-Path $configPath) {
        $content = Get-Content $configPath
        foreach ($line in $content) {
            if ($line -match '^LocalTempPath=(.+)$') {
                $TempPath = $matches[1].Trim()
                break
            }
        }
    }
}

# Check if background.tif exists
# Note: Full validation would check transform matrix in TIFF metadata
$backgroundPath = Join-Path $SharedPath "background.tif"
$hasBackground = Test-Path $backgroundPath

# Check if LUTX files exist
$hasLutx = $false
if (-not [string]::IsNullOrEmpty($TempPath) -and (Test-Path $TempPath)) {
    $lutxFiles = Get-ChildItem -Path $TempPath -Filter "*.lutx" -ErrorAction SilentlyContinue
    $hasLutx = $lutxFiles.Count -gt 0
}

# System is calibrated if BOTH conditions are met
if ($hasBackground -and $hasLutx) {
    exit 0  # GREEN - fully calibrated
} else {
    exit 1  # RED - needs calibration
}

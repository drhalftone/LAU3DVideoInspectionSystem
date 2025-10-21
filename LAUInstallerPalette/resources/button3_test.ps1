# Button 3 Status Check: Label Cameras
# Returns: 0 (GREEN) if at least 2 camera positions are labeled in systemConfig.ini, 1 (RED) otherwise

param(
    [string]$InstallPath = "C:\LAU3DVideoInspectionTools"
)

$configPath = Join-Path $InstallPath "systemConfig.ini"

if (-not (Test-Path $configPath)) {
    exit 1  # RED - config file doesn't exist
}

# Read the INI file and count camera positions
$inCameraSection = $false
$positionCount = 0

Get-Content $configPath | ForEach-Object {
    $line = $_.Trim()

    if ($line -eq "[CameraPosition]") {
        $inCameraSection = $true
        return
    }

    if ($line -match '^\[.*\]$') {
        $inCameraSection = $false
        return
    }

    if ($inCameraSection -and $line -match '=') {
        $positionCount++
    }
}

# Need at least 2 camera positions (for the 2 Lucid cameras)
if ($positionCount -ge 2) {
    exit 0  # GREEN - cameras are labeled
} else {
    exit 1  # RED - cameras not labeled
}

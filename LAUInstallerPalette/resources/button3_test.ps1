# Button 3: Camera Labeling Test
# Purpose: Check if cameras are properly labeled in configuration
# Exit Code: 0 = Ready (GREEN), 1 = Not Ready (RED)

param(
    [Parameter(Mandatory=$true)]
    [string]$InstallPath
)

# Check if cameras are labeled in configuration
$configPath = Join-Path $InstallPath "systemConfig.ini"

if (-not (Test-Path $configPath)) {
    Write-Host "✗ Configuration file not found: $configPath"
    exit 1  # FAIL - no config file
}

# Read and parse the configuration file
$content = Get-Content $configPath -Raw
$cameraSection = $content -match '\[CameraPosition\]([\s\S]*?)(?=\[|\z)'

if ($matches) {
    $positions = ($matches[1] -split "`n" | Where-Object { $_ -match '=' }).Count

    # Need at least 2 camera positions configured
    if ($positions -ge 2) {
        Write-Host "✓ Cameras labeled: $positions positions found"
        exit 0  # SUCCESS - cameras labeled
    } else {
        Write-Host "✗ Not enough camera positions: $positions found (need 2+)"
        exit 1  # FAIL - not enough cameras
    }
}

Write-Host "✗ No camera positions found in configuration"
exit 1  # FAIL - no camera section

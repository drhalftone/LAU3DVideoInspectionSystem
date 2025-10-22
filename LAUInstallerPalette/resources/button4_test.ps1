# Button 4: Background Capture Test
# Purpose: Check if background reference image exists
# Exit Code: 0 = Ready (GREEN), 1 = Not Ready (RED)

param(
    [Parameter(Mandatory=$true)]
    [string]$InstallPath
)

# Define shared data path
$sharedPath = "C:\ProgramData\3DVideoInspectionTools"
$backgroundPath = Join-Path $sharedPath "background.tif"

# Check if background.tif exists
if (Test-Path $backgroundPath) {
    Write-Host "✓ Background image found: $backgroundPath"
    exit 0  # SUCCESS - background exists
} else {
    Write-Host "✗ Background image missing: $backgroundPath"
    exit 1  # FAIL - no background
}

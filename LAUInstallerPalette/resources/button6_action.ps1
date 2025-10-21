# Button 6 Action: Calibrate System
# This script is called when Button 6 is clicked
# Customize this script to perform the action for your button

param(
    [string]$InstallPath = "C:\LAU3DVideoInspectionTools",
    [string]$SharedPath = "C:\Users\Public\Pictures",
    [string]$TempPath = ""
)

# Example: Launch calibration tool
# Start-Process -FilePath (Join-Path $InstallPath "LAU3DVideoCalibrator.exe")

Write-Host "Button 6 action script executed"
Write-Host "InstallPath: $InstallPath"
Write-Host "SharedPath: $SharedPath"
Write-Host "TempPath: $TempPath"

# Return 0 for success, non-zero for failure
exit 0

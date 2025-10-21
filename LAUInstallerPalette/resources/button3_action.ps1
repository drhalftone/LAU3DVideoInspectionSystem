# Button 3 Action: Label Cameras
# This script is called when Button 3 is clicked
# Customize this script to perform the action for your button

param(
    [string]$InstallPath = "C:\LAU3DVideoInspectionTools"
)

# Example: Launch camera labeling tool
# Start-Process -FilePath (Join-Path $InstallPath "LAUBackgroundCapture.exe")

Write-Host "Button 3 action script executed"
Write-Host "InstallPath: $InstallPath"

# Return 0 for success, non-zero for failure
exit 0

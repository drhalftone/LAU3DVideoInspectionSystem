# Button 4 Action: Record Background
# This script is called when Button 4 is clicked
# Customize this script to perform the action for your button

param(
    [string]$InstallPath = "C:\LAU3DVideoInspectionTools",
    [string]$SharedPath = "C:\Users\Public\Pictures"
)

# Example: Launch background capture tool
# Start-Process -FilePath (Join-Path $InstallPath "LAUBackgroundCapture.exe")

Write-Host "Button 4 action script executed"
Write-Host "InstallPath: $InstallPath"
Write-Host "SharedPath: $SharedPath"

# Return 0 for success, non-zero for failure
exit 0

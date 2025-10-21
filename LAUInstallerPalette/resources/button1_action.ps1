# Button 1 Action: System Configuration
# This script is called when Button 1 is clicked
# Customize this script to perform the action for your button

param(
    [string]$InstallPath = "C:\LAU3DVideoInspectionTools"
)

# Example: Launch system configuration tool
# Start-Process -FilePath (Join-Path $InstallPath "LAURemoteToolsScheduler.exe")

Write-Host "Button 1 action script executed"
Write-Host "InstallPath: $InstallPath"

# Return 0 for success, non-zero for failure
exit 0

# Button 2 Action: OnTrak Power Control
# This script is called when Button 2 is clicked
# Customize this script to perform the action for your button

param(
    [string]$InstallPath = "C:\LAU3DVideoInspectionTools"
)

# Example: Launch OnTrak widget
# Start-Process -FilePath (Join-Path $InstallPath "LAUOnTrakWidget.exe")

Write-Host "Button 2 action script executed"
Write-Host "InstallPath: $InstallPath"

# Return 0 for success, non-zero for failure
exit 0

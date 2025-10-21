# Button 5 Action: Monitor Live Video
# This script is called when Button 5 is clicked
# Customize this script to perform the action for your button

param(
    [string]$InstallPath = "C:\LAU3DVideoInspectionTools",
    [string]$SharedPath = "C:\Users\Public\Pictures",
    [string]$TempPath = ""
)

# Example: Show task scheduler dialog or launch monitoring
# Write-Host "Please configure Windows Task Scheduler to run LAUProcessVideos.exe"

Write-Host "Button 5 action script executed"
Write-Host "InstallPath: $InstallPath"
Write-Host "SharedPath: $SharedPath"
Write-Host "TempPath: $TempPath"

# Return 0 for success, non-zero for failure
exit 0

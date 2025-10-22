# Button 2: Power Control Action
# Purpose: Show Task Scheduler setup instructions for OnTrak power control
# This script is executed when the user clicks Button 2

param(
    [Parameter(Mandatory=$true)]
    [string]$InstallPath
)

# Display Task Scheduler setup message
Write-Host "=========================================="
Write-Host "POWER CONTROL - Task Scheduler Setup"
Write-Host "=========================================="
Write-Host ""
Write-Host "OnTrak Power Control needs to run via Windows Task Scheduler."
Write-Host ""
Write-Host "Please configure Task Scheduler:"
Write-Host "1. Press Windows+R and type: taskschd.msc"
Write-Host "2. Click 'Create Task' in the right panel"
Write-Host "3. General tab:"
Write-Host "   - Name: LAUOnTrakWidget"
Write-Host "   - Check: 'Run with highest privileges'"
Write-Host "4. Triggers tab:"
Write-Host "   - New trigger → 'At startup'"
Write-Host "5. Actions tab:"
Write-Host "   - New action → Start a program"
Write-Host "   - Program: $InstallPath\LAUOnTrakWidget.exe"
Write-Host "6. Click OK to save"
Write-Host ""
Write-Host "To start the task now:"
Write-Host "   - Right-click the task and select 'Run'"
Write-Host ""
Write-Host "=========================================="

exit 0

# Button 2: Power Control Test
# Purpose: Check if power control service is running
# Exit Code: 0 = Ready (GREEN), 1 = Not Ready (RED)

param(
    [Parameter(Mandatory=$true)]
    [string]$InstallPath
)

# Check if OnTrak power control process is running
$processName = "LAUOnTrakWidget"
$process = Get-Process -Name $processName -ErrorAction SilentlyContinue

if ($process) {
    Write-Host "✓ Power control running: $processName"
    exit 0  # SUCCESS - Ready (GREEN)
} else {
    Write-Host "✗ Power control not running: $processName"
    exit 1  # FAIL - Not Ready (RED)
}

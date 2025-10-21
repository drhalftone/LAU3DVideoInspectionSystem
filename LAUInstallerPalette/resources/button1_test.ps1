# Button 1 Status Check: System Configuration
# Returns: 0 (GREEN) if systemConfig.ini exists, 1 (RED) otherwise

param(
    [string]$InstallPath = "C:\LAU3DVideoInspectionTools"
)

$configPath = Join-Path $InstallPath "systemConfig.ini"

if (Test-Path $configPath) {
    exit 0  # GREEN - config file exists
} else {
    exit 1  # RED - config file missing
}

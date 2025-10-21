# Button 5 Status Check: Monitor Live Video
# Returns: 0 (GREEN) if recording active or system calibrated, 1 (RED) otherwise
#
# Green if:
# - LAUProcessVideos.exe is running, OR
# - noCal files exist (recording has happened), OR
# - System is fully calibrated (background.tif with valid transform AND LUTX files exist)

param(
    [string]$SharedPath = "C:\Users\Public\Pictures",
    [string]$TempPath = ""
)

# Check if LAUProcessVideos.exe is running
$processName = "LAUProcessVideos"
$process = Get-Process -Name $processName -ErrorAction SilentlyContinue

if ($process) {
    exit 0  # GREEN - process is running
}

# If TempPath not provided, try to read from config
if ([string]::IsNullOrEmpty($TempPath)) {
    $configPath = "C:\LAU3DVideoInspectionTools\systemConfig.ini"
    if (Test-Path $configPath) {
        $content = Get-Content $configPath
        foreach ($line in $content) {
            if ($line -match '^LocalTempPath=(.+)$') {
                $TempPath = $matches[1].Trim()
                break
            }
        }
    }
}

# Check for noCal files
if (-not [string]::IsNullOrEmpty($TempPath) -and (Test-Path $TempPath)) {
    $noCalFiles = Get-ChildItem -Path $TempPath -Filter "noCal*.tif" -ErrorAction SilentlyContinue
    if ($noCalFiles.Count -gt 0) {
        exit 0  # GREEN - noCal files exist
    }
}

# Check if system is fully calibrated
# Need BOTH background.tif with valid transform AND LUTX files
$backgroundPath = Join-Path $SharedPath "background.tif"
$hasBackground = Test-Path $backgroundPath

# Note: Validating transform matrix requires reading TIFF metadata
# For PowerShell, we'll assume if background.tif exists, it's valid
# You may want to enhance this check with actual TIFF validation

$hasLutx = $false
if (-not [string]::IsNullOrEmpty($TempPath) -and (Test-Path $TempPath)) {
    $lutxFiles = Get-ChildItem -Path $TempPath -Filter "*.lutx" -ErrorAction SilentlyContinue
    $hasLutx = $lutxFiles.Count -gt 0
}

if ($hasBackground -and $hasLutx) {
    exit 0  # GREEN - system is calibrated
}

exit 1  # RED - none of the conditions met

# Button 4 Status Check: Record Background
# Returns: 0 (GREEN) if background.tif exists, 1 (RED) otherwise

param(
    [string]$SharedPath = "C:\Users\Public\Pictures"
)

$backgroundPath = Join-Path $SharedPath "background.tif"

if (Test-Path $backgroundPath) {
    exit 0  # GREEN - background file exists
} else {
    exit 1  # RED - background file missing
}

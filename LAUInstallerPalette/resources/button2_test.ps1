# Button 2 Status Check: OnTrak Power Control
# Returns: 0 (GREEN) if LAUOnTrakWidget.exe is running, 1 (RED) otherwise

$processName = "LAUOnTrakWidget"
$process = Get-Process -Name $processName -ErrorAction SilentlyContinue

if ($process) {
    exit 0  # GREEN - OnTrak is running
} else {
    exit 1  # RED - OnTrak is not running
}

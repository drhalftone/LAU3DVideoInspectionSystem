<#
.SYNOPSIS
    Configures Windows for unattended PC operation with auto-login resilience.

.DESCRIPTION
    This script configures Windows 10/11 to handle updates and restarts without
    requiring physical keyboard/mouse/monitor access. Essential for remote
    recording systems accessed via Chrome Remote Desktop.

    Features:
    - Prevents automatic restarts during active hours
    - Skips post-update OOBE/privacy screens
    - Disables lock screen and login animations
    - Creates auto-login watchdog to re-enable if updates disable it
    - Configures update policies for unattended operation

.PARAMETER Username
    Windows username for auto-login watchdog. If not provided, prompts user.

.PARAMETER Password
    Windows password for auto-login watchdog. If not provided, prompts user.

.PARAMETER ActiveHoursStart
    Start of active hours (0-23). Default: 6 (6 AM)

.PARAMETER ActiveHoursEnd
    End of active hours (0-23). Default: 18 (6 PM)

.PARAMETER SkipAutoLoginWatchdog
    Skip creating the auto-login watchdog task.

.EXAMPLE
    .\Configure-UnattendedPC.ps1
    Prompts for username/password and configures with default active hours.

.EXAMPLE
    .\Configure-UnattendedPC.ps1 -Username "User" -Password "SecurePass123"
    Configures with specified credentials.

.EXAMPLE
    .\Configure-UnattendedPC.ps1 -ActiveHoursStart 5 -ActiveHoursEnd 20
    Sets active hours from 5 AM to 8 PM.

.EXAMPLE
    .\Configure-UnattendedPC.ps1 -SkipAutoLoginWatchdog
    Configures update settings but skips auto-login watchdog creation.

.NOTES
    Author: Lau Consulting Inc
    Requires: Administrator privileges
    Compatible: Windows 10/11
#>

param(
    [Parameter(Mandatory=$false)]
    [string]$Username,

    [Parameter(Mandatory=$false)]
    [string]$Password,

    [Parameter(Mandatory=$false)]
    [ValidateRange(0, 23)]
    [int]$ActiveHoursStart = 6,

    [Parameter(Mandatory=$false)]
    [ValidateRange(0, 23)]
    [int]$ActiveHoursEnd = 18,

    [Parameter(Mandatory=$false)]
    [switch]$SkipAutoLoginWatchdog
)

#Requires -RunAsAdministrator

# ============================================================================
# BANNER
# ============================================================================
Write-Host ""
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "  Configure Unattended PC - Windows Update Resilience" -ForegroundColor Cyan
Write-Host "  Lau Consulting Inc | drhalftone.com" -ForegroundColor Cyan
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host ""

# ============================================================================
# VALIDATION
# ============================================================================
if ($ActiveHoursEnd -le $ActiveHoursStart) {
    Write-Host "ERROR: ActiveHoursEnd must be greater than ActiveHoursStart" -ForegroundColor Red
    exit 1
}

# ============================================================================
# CONFIGURE WINDOWS UPDATE POLICIES
# ============================================================================
Write-Host "[1/6] Configuring Windows Update policies..." -ForegroundColor Yellow

# Prevent automatic restart with logged-on users
New-Item -Path "HKLM:\SOFTWARE\Policies\Microsoft\Windows\WindowsUpdate" -Force | Out-Null
New-Item -Path "HKLM:\SOFTWARE\Policies\Microsoft\Windows\WindowsUpdate\AU" -Force | Out-Null
Set-ItemProperty -Path "HKLM:\SOFTWARE\Policies\Microsoft\Windows\WindowsUpdate\AU" -Name "NoAutoRebootWithLoggedOnUsers" -Value 1 -Type DWord
Write-Host "  ✓ Disabled auto-restart with logged-on users" -ForegroundColor Green

# Configure automatic updates to download and schedule install
Set-ItemProperty -Path "HKLM:\SOFTWARE\Policies\Microsoft\Windows\WindowsUpdate\AU" -Name "AUOptions" -Value 4 -Type DWord
Write-Host "  ✓ Set updates to download and schedule install" -ForegroundColor Green

# ============================================================================
# SET ACTIVE HOURS
# ============================================================================
Write-Host ""
Write-Host "[2/6] Setting active hours ($ActiveHoursStart:00 - $ActiveHoursEnd:00)..." -ForegroundColor Yellow

New-Item -Path "HKLM:\SOFTWARE\Microsoft\WindowsUpdate\UX\Settings" -Force | Out-Null
Set-ItemProperty -Path "HKLM:\SOFTWARE\Microsoft\WindowsUpdate\UX\Settings" -Name "ActiveHoursStart" -Value $ActiveHoursStart -Type DWord
Set-ItemProperty -Path "HKLM:\SOFTWARE\Microsoft\WindowsUpdate\UX\Settings" -Name "ActiveHoursEnd" -Value $ActiveHoursEnd -Type DWord
Write-Host "  ✓ Windows will avoid restarting during these hours" -ForegroundColor Green

# ============================================================================
# SKIP POST-UPDATE OOBE SCREENS
# ============================================================================
Write-Host ""
Write-Host "[3/6] Disabling post-update setup screens..." -ForegroundColor Yellow

# Disable privacy experience after updates
New-Item -Path "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\OOBE" -Force | Out-Null
Set-ItemProperty -Path "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\OOBE" -Name "DisablePrivacyExperience" -Value 1 -Type DWord
Write-Host "  ✓ Disabled privacy experience screens" -ForegroundColor Green

# Skip post-update setup screens
New-Item -Path "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Setup\OOBE" -Force | Out-Null
Set-ItemProperty -Path "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Setup\OOBE" -Name "UnattendCreatedUser" -Value 1 -Type DWord
Write-Host "  ✓ Disabled 'Let's finish setting up' screens" -ForegroundColor Green

# Mark EULA as displayed
Set-ItemProperty -Path "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\OOBE" -Name "SetupDisplayedEula" -Value 1 -Type DWord
Write-Host "  ✓ Marked EULA as displayed" -ForegroundColor Green

# ============================================================================
# DISABLE LOGIN ANIMATIONS AND LOCK SCREEN
# ============================================================================
Write-Host ""
Write-Host "[4/6] Disabling login animations and lock screen..." -ForegroundColor Yellow

# Disable "Hi" animation and first logon animation
New-Item -Path "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Policies\System" -Force | Out-Null
Set-ItemProperty -Path "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Policies\System" -Name "EnableFirstLogonAnimation" -Value 0 -Type DWord
Write-Host "  ✓ Disabled first logon animation" -ForegroundColor Green

# Disable lock screen (goes straight to login)
New-Item -Path "HKLM:\SOFTWARE\Policies\Microsoft\Windows\Personalization" -Force | Out-Null
Set-ItemProperty -Path "HKLM:\SOFTWARE\Policies\Microsoft\Windows\Personalization" -Name "NoLockScreen" -Value 1 -Type DWord
Write-Host "  ✓ Disabled lock screen" -ForegroundColor Green

# ============================================================================
# OPTIMIZE SHUTDOWN/STARTUP BEHAVIOR
# ============================================================================
Write-Host ""
Write-Host "[5/6] Optimizing shutdown and startup behavior..." -ForegroundColor Yellow

# Disable forced restarts
Set-ItemProperty -Path "HKLM:\SOFTWARE\Policies\Microsoft\Windows\WindowsUpdate\AU" -Name "NoAutoUpdate" -Value 0 -Type DWord
Write-Host "  ✓ Configured update behavior" -ForegroundColor Green

# Reduce shutdown timeout for hung applications
Set-ItemProperty -Path "HKCU:\Control Panel\Desktop" -Name "WaitToKillAppTimeout" -Value 2000 -Type String -ErrorAction SilentlyContinue
Set-ItemProperty -Path "HKLM:\SYSTEM\CurrentControlSet\Control" -Name "WaitToKillServiceTimeout" -Value 2000 -Type String
Write-Host "  ✓ Reduced shutdown timeouts" -ForegroundColor Green

# ============================================================================
# CREATE AUTO-LOGIN WATCHDOG
# ============================================================================
Write-Host ""
if ($SkipAutoLoginWatchdog) {
    Write-Host "[6/6] Skipping auto-login watchdog (per user request)" -ForegroundColor Yellow
} else {
    Write-Host "[6/6] Creating auto-login watchdog task..." -ForegroundColor Yellow

    # Prompt for credentials if not provided
    if ([string]::IsNullOrWhiteSpace($Username)) {
        $Username = Read-Host "Enter Windows username for auto-login"
    }

    if ([string]::IsNullOrWhiteSpace($Password)) {
        $SecurePassword = Read-Host "Enter Windows password for auto-login" -AsSecureString
        $BSTR = [System.Runtime.InteropServices.Marshal]::SecureStringToBSTR($SecurePassword)
        $Password = [System.Runtime.InteropServices.Marshal]::PtrToStringAuto($BSTR)
    }

    # Create watchdog script
    $watchdogScriptPath = "C:\ProgramData\RemoteRecordingTools\EnsureAutoLogin.ps1"
    $watchdogDir = Split-Path $watchdogScriptPath -Parent

    if (!(Test-Path $watchdogDir)) {
        New-Item -ItemType Directory -Path $watchdogDir -Force | Out-Null
    }

    $watchdogScript = @"
# Auto-Login Watchdog - Re-enables auto-login if disabled by Windows Update
# Created by LAURemoteTools

`$username = "$Username"
`$password = "$Password"

try {
    # Re-apply auto-login settings
    Set-ItemProperty -Path "HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon" -Name "AutoAdminLogon" -Value "1" -ErrorAction Stop
    Set-ItemProperty -Path "HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon" -Name "DefaultUserName" -Value `$username -ErrorAction Stop
    Set-ItemProperty -Path "HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon" -Name "DefaultPassword" -Value `$password -ErrorAction Stop

    # Log success
    `$timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
    Add-Content -Path "C:\ProgramData\RemoteRecordingTools\AutoLoginWatchdog.log" -Value "[`$timestamp] Auto-login settings verified/restored"
} catch {
    # Log error
    `$timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
    Add-Content -Path "C:\ProgramData\RemoteRecordingTools\AutoLoginWatchdog.log" -Value "[`$timestamp] ERROR: `$_"
}
"@

    Set-Content -Path $watchdogScriptPath -Value $watchdogScript -Force
    Write-Host "  ✓ Created watchdog script: $watchdogScriptPath" -ForegroundColor Green

    # Create scheduled task
    $taskName = "AutoLogin-Watchdog"

    # Delete existing task if present
    $existingTask = Get-ScheduledTask -TaskName $taskName -ErrorAction SilentlyContinue
    if ($existingTask) {
        Unregister-ScheduledTask -TaskName $taskName -Confirm:$false
    }

    # Create new task
    $action = New-ScheduledTaskAction -Execute "powershell.exe" -Argument "-ExecutionPolicy Bypass -NoProfile -File `"$watchdogScriptPath`""
    $trigger = New-ScheduledTaskTrigger -AtStartup
    $principal = New-ScheduledTaskPrincipal -UserId "SYSTEM" -LogonType ServiceAccount -RunLevel Highest
    $settings = New-ScheduledTaskSettingsSet -AllowStartIfOnBatteries -DontStopIfGoingOnBatteries -StartWhenAvailable

    Register-ScheduledTask -TaskName $taskName -Action $action -Trigger $trigger -Principal $principal -Settings $settings -Description "Re-enables auto-login if disabled by Windows Update" | Out-Null
    Write-Host "  ✓ Created scheduled task: $taskName" -ForegroundColor Green

    # Also apply auto-login settings now
    Set-ItemProperty -Path "HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon" -Name "AutoAdminLogon" -Value "1"
    Set-ItemProperty -Path "HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon" -Name "DefaultUserName" -Value $Username
    Set-ItemProperty -Path "HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon" -Name "DefaultPassword" -Value $Password
    Write-Host "  ✓ Applied auto-login settings immediately" -ForegroundColor Green
}

# ============================================================================
# SUMMARY
# ============================================================================
Write-Host ""
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "  Configuration Complete!" -ForegroundColor Cyan
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Summary of Changes:" -ForegroundColor White
Write-Host "  ✓ Windows Update auto-restart disabled during active hours" -ForegroundColor Green
Write-Host "  ✓ Active hours: $ActiveHoursStart:00 - $ActiveHoursEnd:00" -ForegroundColor Green
Write-Host "  ✓ Post-update OOBE/privacy screens disabled" -ForegroundColor Green
Write-Host "  ✓ Lock screen and login animations disabled" -ForegroundColor Green
Write-Host "  ✓ Shutdown/startup behavior optimized" -ForegroundColor Green

if (!$SkipAutoLoginWatchdog) {
    Write-Host "  ✓ Auto-login watchdog created (runs on every boot)" -ForegroundColor Green
    Write-Host "  ✓ Auto-login enabled for user: $Username" -ForegroundColor Green
}

Write-Host ""
Write-Host "Next Steps:" -ForegroundColor Yellow
Write-Host "  1. Restart the computer to test auto-login" -ForegroundColor White
Write-Host "  2. Verify Chrome Remote Desktop connects after restart" -ForegroundColor White
Write-Host "  3. Check that OneDrive starts automatically" -ForegroundColor White
Write-Host "  4. Monitor for Windows Update behavior" -ForegroundColor White
Write-Host ""

if (!$SkipAutoLoginWatchdog) {
    Write-Host "Auto-Login Watchdog Log:" -ForegroundColor Yellow
    Write-Host "  C:\ProgramData\RemoteRecordingTools\AutoLoginWatchdog.log" -ForegroundColor White
    Write-Host ""
}

Write-Host "⚠️  Security Note:" -ForegroundColor Yellow
Write-Host "  Password is stored in plaintext in registry:" -ForegroundColor White
Write-Host "  HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon" -ForegroundColor White
Write-Host "  Only use on physically secure systems." -ForegroundColor White
Write-Host ""

# Offer to restart
$restart = Read-Host "Would you like to restart now to test the configuration? (Y/N)"
if ($restart -eq "Y" -or $restart -eq "y") {
    Write-Host ""
    Write-Host "Restarting in 10 seconds... Press Ctrl+C to cancel" -ForegroundColor Yellow
    Start-Sleep -Seconds 10
    Restart-Computer -Force
} else {
    Write-Host ""
    Write-Host "Please restart manually when ready." -ForegroundColor White
    Write-Host ""
}

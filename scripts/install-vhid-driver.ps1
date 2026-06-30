# Install deskflow-vhid.sys (test signing or production cert required).
param(
    [string]$DriverDir,
    [switch]$Uninstall
)

$ErrorActionPreference = "Stop"
$RepoRoot = Split-Path $PSScriptRoot -Parent

if (-not $DriverDir) {
    $DriverDir = Join-Path $RepoRoot "build\driver\x64\Release"
}

$Inf = Join-Path $DriverDir "deskflow-vhid.inf"
$Sys = Join-Path $DriverDir "deskflow-vhid.sys"

function Test-IsAdmin {
    $identity = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = [Security.Principal.WindowsPrincipal]$identity
    return $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

if (-not (Test-IsAdmin)) {
    Write-Host "Elevating to install deskflow-vhid driver..."
    $stamp = Join-Path $env:TEMP "deskflow-vhid-install.json"
    @{ DriverDir = $DriverDir; Uninstall = [bool]$Uninstall } | ConvertTo-Json | Set-Content $stamp
    $argList = "-ExecutionPolicy Bypass -File `"$PSCommandPath`" -DriverDir `"$DriverDir`""
    if ($Uninstall) { $argList += " -Uninstall" }
    Start-Process powershell -Verb RunAs -ArgumentList $argList -Wait
    exit $LASTEXITCODE
}

if ($Uninstall) {
    Write-Host "Removing deskflow-vhid driver package..."
    pnputil /delete-driver deskflow-vhid.inf /uninstall /force 2>$null
    sc.exe stop DeskflowVhid 2>$null
    sc.exe delete DeskflowVhid 2>$null
    Write-Host "Done."
    exit 0
}

if (-not (Test-Path $Sys)) {
    Write-Host "Building driver first..."
    & (Join-Path $RepoRoot "scripts\build-vhid-driver.ps1")
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

if (-not (Test-Path $Inf)) {
    throw "Missing $Inf — copy deskflow-vhid.inf beside deskflow-vhid.sys in $DriverDir"
}

# Stage INF + SYS together for pnputil.
$Stage = Join-Path $env:TEMP "deskflow-vhid-stage"
if (Test-Path $Stage) { Remove-Item $Stage -Recurse -Force }
New-Item -ItemType Directory -Path $Stage | Out-Null
Copy-Item $Sys, (Join-Path $RepoRoot "src\driver\deskflow-vhid\deskflow-vhid.inf") $Stage

Write-Host "Installing driver from $Stage ..."
pnputil /add-driver (Join-Path $Stage "deskflow-vhid.inf") /install

Write-Host ""
Write-Host "Next steps:"
Write-Host "  1. If install failed with signature error, enable test signing:"
Write-Host "       bcdedit /set testsigning on   (reboot required)"
Write-Host "  2. Create the root-enumerated device (Device Manager -> Add legacy hardware),"
Write-Host "     or reboot after pnputil if the devnode appears automatically."
Write-Host "  3. Run: deskflow-vhid-bridge.exe test"
Write-Host "  4. Trigger UAC and use: move / click commands to verify virtual HID"

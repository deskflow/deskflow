#requires -Version 5.1
<#
.SYNOPSIS
  Quit Deskflow, install the Release build to Program Files, restart service + GUI.
.DESCRIPTION
  Stops the Deskflow Windows service and all deskflow*.exe processes, runs
  cmake --install into DESKFLOW_INSTALL_DIR (default C:\Program Files\Deskflow),
  registers or updates the Deskflow service (deskflow-daemon.exe), then starts
  the service and launches deskflow.exe from the install directory.

  Re-launches elevated when installing under Program Files without admin rights.
.EXAMPLE
  pwsh scripts\install-windows.ps1
  pwsh scripts\install-windows.ps1 -NoRestart
#>
param(
  [switch]$NoRestart,
  [string]$BuildDir,
  [string]$InstallDir
)

$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent

function Import-DeskflowEnv {
  $envFile = Join-Path $root '.env'
  if (-not (Test-Path $envFile)) { return }
  Get-Content $envFile | ForEach-Object {
    if ($_ -match '^\s*([A-Za-z_][A-Za-z0-9_]*)\s*=\s*(.*)$' -and $_ -notmatch '^\s*#') {
      Set-Item -Path "env:$($matches[1])" -Value ($matches[2].Trim())
    }
  }
}

function Test-IsAdmin {
  $identity = [Security.Principal.WindowsIdentity]::GetCurrent()
  $principal = New-Object Security.Principal.WindowsPrincipal($identity)
  return $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

function Assert-Admin {
  if (Test-IsAdmin) { return }
  Write-Host 'Re-launching elevated for Program Files install...'
  $argList = @(
    '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', $MyInvocation.MyCommand.Path
  )
  if ($NoRestart) { $argList += '-NoRestart' }
  if ($BuildDir) { $argList += @('-BuildDir', $BuildDir) }
  if ($InstallDir) { $argList += @('-InstallDir', $InstallDir) }
  $proc = Start-Process -FilePath 'powershell.exe' -Verb RunAs -ArgumentList $argList -Wait -PassThru
  exit $proc.ExitCode
}

function Stop-DeskflowAll {
  Write-Host '== Stopping Deskflow processes and service =='
  Stop-Service -Name Deskflow -Force -ErrorAction SilentlyContinue
  Start-Sleep -Seconds 2

  $names = @('deskflow', 'deskflow-core', 'deskflow-daemon', 'deskflow-vhid-bridge')
  foreach ($round in 1..3) {
    $any = $false
    foreach ($name in $names) {
      $procs = Get-Process -Name $name -ErrorAction SilentlyContinue
      if ($procs) {
        $any = $true
        $procs | Stop-Process -Force -ErrorAction SilentlyContinue
      }
    }
    if (-not $any) { break }
    Start-Sleep -Seconds 1
  }
  foreach ($name in $names) {
    Get-Process -Name $name -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue
  }
  Start-Sleep -Seconds 1
}

function Ensure-DeskflowService {
  param([string]$DaemonPath)
  if (-not (Test-Path $DaemonPath)) {
    throw "deskflow-daemon.exe not found at $DaemonPath"
  }
  $binPath = "`"$DaemonPath`""
  $svc = Get-Service -Name Deskflow -ErrorAction SilentlyContinue
  if (-not $svc) {
    Write-Host "== Creating Deskflow Windows service =="
    sc.exe create Deskflow binPath= $binPath start= auto DisplayName= "Deskflow" | Out-Null
    if ($LASTEXITCODE -ne 0) { throw "sc.exe create failed ($LASTEXITCODE)" }
  } else {
    Write-Host "== Updating Deskflow Windows service binary path =="
    sc.exe config Deskflow binPath= $binPath | Out-Null
    if ($LASTEXITCODE -ne 0) { throw "sc.exe config failed ($LASTEXITCODE)" }
  }
}

function Start-DeskflowService {
  $svc = Get-Service -Name Deskflow -ErrorAction SilentlyContinue
  if (-not $svc) { return }
  if ($svc.Status -ne 'Running') {
    Write-Host '== Starting Deskflow service =='
    Start-Service Deskflow
  }
  Write-Host ("== Service status: " + (Get-Service Deskflow).Status + " ==")
}

function Start-DeskflowGui {
  param([string]$InstallRoot)
  $gui = Join-Path $InstallRoot 'deskflow.exe'
  if (-not (Test-Path $gui)) { throw "deskflow.exe not found at $gui" }
  Write-Host "== Launching $gui =="
  Start-Process -FilePath $gui -WorkingDirectory $InstallRoot
}

Import-DeskflowEnv
Assert-Admin

if (-not $BuildDir) {
  $BuildDir = if ($env:DESKFLOW_BUILD_DIR) { $env:DESKFLOW_BUILD_DIR } else { 'build' }
}
if (-not $InstallDir) {
  $InstallDir = if ($env:DESKFLOW_INSTALL_DIR) { $env:DESKFLOW_INSTALL_DIR } else { Join-Path ${env:ProgramFiles} 'Deskflow' }
}
$buildPath = if ([System.IO.Path]::IsPathRooted($BuildDir)) { $BuildDir } else { Join-Path $root $BuildDir }

if (-not (Test-Path (Join-Path $buildPath 'cmake_install.cmake'))) {
  throw "Build tree not found at $buildPath — configure and build first."
}

Stop-DeskflowAll

Write-Host "== Installing to $InstallDir =="
if (Test-Path $InstallDir) {
  $backup = "$InstallDir.bak"
  if (Test-Path $backup) { Remove-Item -Recurse -Force $backup }
  Rename-Item -Path $InstallDir -NewName (Split-Path $backup -Leaf) -ErrorAction SilentlyContinue
  if (Test-Path $InstallDir) { Remove-Item -Recurse -Force $InstallDir }
  Write-Host "Backed up previous install to $backup"
}
New-Item -ItemType Directory -Force -Path $InstallDir | Out-Null

cmake --install $buildPath --config Release --prefix $InstallDir
if ($LASTEXITCODE -ne 0) { throw "cmake --install failed ($LASTEXITCODE)" }

$daemon = Join-Path $InstallDir 'deskflow-daemon.exe'
Ensure-DeskflowService -DaemonPath $daemon
Start-DeskflowService

if (-not $NoRestart) {
  Start-DeskflowGui -InstallRoot $InstallDir
}

Write-Host "== Done: $InstallDir =="

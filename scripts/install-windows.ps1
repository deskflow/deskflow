#requires -Version 5.1
<#
.SYNOPSIS
  Quit Deskflow, install the Release build to Program Files, restart service + GUI.
.DESCRIPTION
  Stops every Deskflow process (any path), removes rogue install copies, copies the
  windeployqt-staged build from build/bin/Release into DESKFLOW_INSTALL_DIR,
  registers the Deskflow service, starts it once, and launches a single deskflow.exe
  from the canonical install directory.

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

$script:DeskflowProcessNames = @(
  'deskflow', 'deskflow-core', 'deskflow-daemon', 'deskflow-vhid-bridge'
)

function Import-DeskflowEnv {
  $envFile = Join-Path $root '.env'
  if (-not (Test-Path $envFile)) { return }
  Get-Content $envFile | ForEach-Object {
    if ($_ -match '^\s*([A-Za-z_][A-Za-z0-9_]*)\s*=\s*(.*)$' -and $_ -notmatch '^\s*#') {
      Set-Item -Path "env:$($matches[1])" -Value ($matches[2].Trim().Trim('"'))
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
  $scriptPath = if ($PSCommandPath) { $PSCommandPath } else { $MyInvocation.MyCommand.Path }
  if (-not $scriptPath) { throw 'Could not resolve install script path for elevation.' }
  $argList = @(
    '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', $scriptPath
  )
  if ($NoRestart) { $argList += '-NoRestart' }
  if ($BuildDir) { $argList += @('-BuildDir', $BuildDir) }
  if ($InstallDir) { $argList += @('-InstallDir', $InstallDir) }
  $proc = Start-Process -FilePath 'powershell.exe' -Verb RunAs -ArgumentList $argList -Wait -PassThru
  exit $proc.ExitCode
}

function Get-DeskflowProcesses {
  Get-CimInstance Win32_Process -Filter "Name LIKE 'deskflow%'" -ErrorAction SilentlyContinue
}

function Stop-DeskflowAll {
  Write-Host '== Stopping Deskflow service and all processes =='

  $svc = Get-Service -Name Deskflow -ErrorAction SilentlyContinue
  if ($svc -and $svc.Status -eq 'Running') {
    Stop-Service -Name Deskflow -Force -ErrorAction SilentlyContinue
    Start-Sleep -Seconds 2
  }
  if (Get-Service -Name Deskflow -ErrorAction SilentlyContinue) {
    sc.exe stop Deskflow 2>$null | Out-Null
    Start-Sleep -Seconds 1
    sc.exe delete Deskflow 2>$null | Out-Null
    Start-Sleep -Seconds 1
  }

  $deadline = (Get-Date).AddSeconds(25)
  while ((Get-Date) -lt $deadline) {
    $procs = @(Get-DeskflowProcesses)
    if ($procs.Count -eq 0) { break }

    foreach ($proc in $procs) {
      Write-Host "  stopping PID $($proc.ProcessId) $($proc.Name) ($($proc.ExecutablePath))"
      Stop-Process -Id $proc.ProcessId -Force -ErrorAction SilentlyContinue
    }

    foreach ($name in $script:DeskflowProcessNames) {
      Get-Process -Name $name -ErrorAction SilentlyContinue |
        Stop-Process -Force -ErrorAction SilentlyContinue
      taskkill /F /T /IM "$name.exe" 2>$null | Out-Null
    }

    Start-Sleep -Milliseconds 750
  }

  $remaining = @(Get-DeskflowProcesses)
  if ($remaining.Count -gt 0) {
    $detail = ($remaining | ForEach-Object { "$($_.Name) pid=$($_.ProcessId) path=$($_.ExecutablePath)" }) -join '; '
    throw "Could not stop all Deskflow processes: $detail"
  }

  Write-Host '== All Deskflow processes stopped =='
}

function Get-RogueInstallPaths {
  param([string]$CanonicalDir)

  $paths = [System.Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
  foreach ($candidate in @(
      (Join-Path $env:LOCALAPPDATA 'Programs\Deskflow'),
      (Join-Path ${env:ProgramFiles(x86)} 'Deskflow'),
      'C:\Program'
    )) {
    if ($candidate -and (Test-Path (Join-Path $candidate 'deskflow.exe'))) {
      [void]$paths.Add([System.IO.Path]::GetFullPath($candidate))
    }
  }

  $canonical = [System.IO.Path]::GetFullPath($CanonicalDir)
  return @($paths | Where-Object { $_ -ne $canonical })
}

function Remove-RogueInstalls {
  param(
    [string[]]$Paths,
    [string]$CanonicalDir
  )

  if ($Paths.Count -eq 0) { return }

  Stop-DeskflowAll
  foreach ($rogue in $Paths) {
    Write-Host "Removing rogue install: $rogue"
    Remove-Item -LiteralPath $rogue -Recurse -Force -ErrorAction SilentlyContinue
  }
}

function Ensure-DeskflowService {
  param([string]$DaemonPath)

  if (-not (Test-Path $DaemonPath)) {
    throw "deskflow-daemon.exe not found at $DaemonPath"
  }

  $binPath = "`"$DaemonPath`""
  if (Get-Service -Name Deskflow -ErrorAction SilentlyContinue) {
    Write-Host '== Updating Deskflow Windows service =='
    sc.exe config Deskflow binPath= $binPath start= auto | Out-Null
    if ($LASTEXITCODE -ne 0) { throw "sc.exe config failed ($LASTEXITCODE)" }
  } else {
    Write-Host '== Creating Deskflow Windows service =='
    sc.exe create Deskflow binPath= $binPath start= auto DisplayName= "Deskflow" | Out-Null
    if ($LASTEXITCODE -ne 0) { throw "sc.exe create failed ($LASTEXITCODE)" }
  }
}

function Start-DeskflowService {
  $svc = Get-Service -Name Deskflow -ErrorAction SilentlyContinue
  if (-not $svc) { return }

  if ($svc.Status -eq 'Running') {
    Write-Host '== Deskflow service already running; restarting =='
    Restart-Service Deskflow -Force
  } else {
    Write-Host '== Starting Deskflow service =='
    Start-Service Deskflow
  }
  Write-Host ("== Service status: " + (Get-Service Deskflow).Status + " ==")
}

function Set-DeskflowRunRegistry {
  param([string]$GuiPath)

  $runKey = 'HKCU:\SOFTWARE\Microsoft\Windows\CurrentVersion\Run'
  $target = "`"$GuiPath`""
  $existing = (Get-ItemProperty -Path $runKey -Name 'Deskflow' -ErrorAction SilentlyContinue).Deskflow
  if ($existing -ne $target) {
    Write-Host "Updating login startup entry -> $GuiPath"
    Set-ItemProperty -Path $runKey -Name 'Deskflow' -Value $target
  }
}

function Start-DeskflowGui {
  param([string]$InstallRoot)

  $gui = Join-Path $InstallRoot 'deskflow.exe'
  if (-not (Test-Path $gui)) { throw "deskflow.exe not found at $gui" }

  $canonicalGui = [System.IO.Path]::GetFullPath($gui).ToLowerInvariant()

  foreach ($proc in @(Get-DeskflowProcesses)) {
    $path = if ($proc.ExecutablePath) { $proc.ExecutablePath.ToLowerInvariant() } else { '' }
    if ($proc.Name -ieq 'deskflow.exe' -and $path -eq $canonicalGui) {
      Write-Host "== deskflow.exe already running from $InstallRoot; not launching a second GUI =="
      return
    }
    if ($proc.Name -ieq 'deskflow.exe') {
      Write-Host "  stopping extra GUI PID $($proc.ProcessId) ($($proc.ExecutablePath))"
      Stop-Process -Id $proc.ProcessId -Force -ErrorAction SilentlyContinue
    }
  }

  Start-Sleep -Seconds 1
  if (Get-Process -Name deskflow -ErrorAction SilentlyContinue) {
    throw 'deskflow.exe still running after cleanup; refusing to launch another instance.'
  }

  Write-Host "== Launching single GUI: $gui --show =="
  Start-Process -FilePath $gui -WorkingDirectory $InstallRoot -ArgumentList '--show'
}

function Assert-CanonicalRuntime {
  param([string]$InstallDir)

  $canonical = [System.IO.Path]::GetFullPath($InstallDir).ToLowerInvariant()
  $foreign = @(Get-DeskflowProcesses | Where-Object {
      $_.ExecutablePath -and
      (-not ($_.ExecutablePath.ToLowerInvariant().StartsWith($canonical)))
    })

  if ($foreign.Count -gt 0) {
    $detail = ($foreign | ForEach-Object { "$($_.Name) $($_.ExecutablePath)" }) -join '; '
    throw "Deskflow still running from non-canonical paths: $detail"
  }
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
$releaseDir = Join-Path $buildPath 'bin\Release'
$InstallDir = [System.IO.Path]::GetFullPath($InstallDir)

if (-not (Test-Path (Join-Path $releaseDir 'deskflow.exe'))) {
  throw "Release build not found at $releaseDir - configure and build first."
}

Stop-DeskflowAll

$roguePaths = Get-RogueInstallPaths -CanonicalDir $InstallDir
Remove-RogueInstalls -Paths $roguePaths -CanonicalDir $InstallDir

Write-Host "== Installing to $InstallDir =="
if (Test-Path $InstallDir) {
  Remove-Item -LiteralPath $InstallDir -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $InstallDir | Out-Null

Write-Host "Copying runtime from $releaseDir..."
Copy-Item -Path (Join-Path $releaseDir '*') -Destination $InstallDir -Recurse -Force
Remove-Item (Join-Path $InstallDir 'legacytests.exe') -Force -ErrorAction SilentlyContinue

$srcWidgets = Get-Item -LiteralPath (Join-Path $releaseDir 'Qt6Widgets.dll')
$dstWidgets = Get-Item -LiteralPath (Join-Path $InstallDir 'Qt6Widgets.dll')
if ($srcWidgets.Length -ne $dstWidgets.Length) {
  throw "Qt runtime mismatch after install (expected $($srcWidgets.Length) bytes, got $($dstWidgets.Length))."
}

$daemon = Join-Path $InstallDir 'deskflow-daemon.exe'
$gui = Join-Path $InstallDir 'deskflow.exe'

Ensure-DeskflowService -DaemonPath $daemon
Set-DeskflowRunRegistry -GuiPath $gui
Start-DeskflowService

if (-not $NoRestart) {
  Start-DeskflowGui -InstallRoot $InstallDir
}

Assert-CanonicalRuntime -InstallDir $InstallDir

Write-Host "== Done: single install at $InstallDir =="
$svcPath = (Get-CimInstance Win32_Service -Filter "Name='Deskflow'" -ErrorAction SilentlyContinue).PathName
Write-Host "== Service: $svcPath =="

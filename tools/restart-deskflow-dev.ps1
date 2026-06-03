param(
  [string]$BuildDir = "build/windows-debug-vcpkg",
  [string]$BuildTarget = "deskflow-core",
  [switch]$BuildGui,
  [switch]$NoGuiBuild,
  [switch]$NoDaemon,
  [switch]$DirectCore,
  [switch]$NoCore,
  [switch]$StopAll
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Test-IsAdministrator {
  $Identity = [Security.Principal.WindowsIdentity]::GetCurrent()
  $Principal = [Security.Principal.WindowsPrincipal]::new($Identity)
  return $Principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

function Quote-ProcessArgument {
  param([Parameter(Mandatory = $true)][string]$Value)

  if ($Value -match '[\s"]') {
    return '"' + $Value.Replace('"', '\"') + '"'
  }

  return $Value
}

if ($StopAll -and -not (Test-IsAdministrator)) {
  $RelaunchArgs = @("-ExecutionPolicy", "Bypass", "-File", (Quote-ProcessArgument -Value $PSCommandPath))
  foreach ($Parameter in $PSBoundParameters.GetEnumerator()) {
    if ($Parameter.Value -is [System.Management.Automation.SwitchParameter]) {
      if ($Parameter.Value.IsPresent) {
        $RelaunchArgs += "-$($Parameter.Key)"
      }
    } else {
      $RelaunchArgs += "-$($Parameter.Key)"
      $RelaunchArgs += Quote-ProcessArgument -Value ([string]$Parameter.Value)
    }
  }

  $WorkingDirectory = Resolve-Path (Join-Path $PSScriptRoot "..")
  Write-Host "Restarting elevated because -StopAll must stop elevated Deskflow processes"
  $ElevatedProcess = Start-Process -FilePath "powershell.exe" -ArgumentList ($RelaunchArgs -join " ") -WorkingDirectory $WorkingDirectory -Verb RunAs -Wait -PassThru
  exit $ElevatedProcess.ExitCode
}

$RepoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$BuildPath = Resolve-Path (Join-Path $RepoRoot $BuildDir)
$BinPath = Join-Path $BuildPath "bin"
$CachePath = Join-Path $BuildPath "CMakeCache.txt"
$AppName = "Deskflow"

if (-not (Test-Path $CachePath)) {
  throw "CMake cache not found: $CachePath"
}

$CMakeExe = $null
foreach ($Line in Get-Content $CachePath) {
  if ($Line -match "^CMAKE_COMMAND:INTERNAL=(.+)$") {
    $CMakeExe = $Matches[1]
    break
  }
}

if (-not $CMakeExe -or -not (Test-Path $CMakeExe)) {
  $CMakeExe = "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
}

if (-not (Test-Path $CMakeExe)) {
  throw "cmake.exe not found: $CMakeExe"
}

$VsDevCmd = "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"
if (-not (Test-Path $VsDevCmd)) {
  throw "VsDevCmd.bat not found: $VsDevCmd"
}

function Set-AsciiIniValueBytes {
  param(
    [Parameter(Mandatory = $true)][string]$Path,
    [Parameter(Mandatory = $true)][string]$Section,
    [Parameter(Mandatory = $true)][string]$Key,
    [Parameter(Mandatory = $true)][string]$Value
  )

  $Encoding = [System.Text.Encoding]::GetEncoding(28591)
  $Bytes = [System.IO.File]::ReadAllBytes($Path)
  $Text = $Encoding.GetString($Bytes)
  $SectionPattern = "(?ms)^\[$([regex]::Escape($Section))\]\r?\n(?<body>.*?)(?=^\[|\z)"
  $KeyPattern = "(?m)^$([regex]::Escape($Key))=.*$"
  $NewLine = "$Key=$Value"
  $SectionMatch = [regex]::Match($Text, $SectionPattern)

  if ($SectionMatch.Success) {
    $SectionText = $SectionMatch.Value
    if ($SectionText -match $KeyPattern) {
      $UpdatedSection = [regex]::Replace($SectionText, $KeyPattern, $NewLine, 1)
    } else {
      $UpdatedSection = $SectionText.TrimEnd("`r", "`n") + "`r`n$NewLine`r`n"
    }
    $Text =
        $Text.Substring(0, $SectionMatch.Index) + $UpdatedSection +
        $Text.Substring($SectionMatch.Index + $SectionText.Length)
  } else {
    $Text = $Text.TrimEnd("`r", "`n") + "`r`n`r`n[$Section]`r`n$NewLine`r`n"
  }

  [System.IO.File]::WriteAllBytes($Path, $Encoding.GetBytes($Text))
}

function Get-IniValue {
  param(
    [Parameter(Mandatory = $true)][string]$Path,
    [Parameter(Mandatory = $true)][string]$Section,
    [Parameter(Mandatory = $true)][string]$Key
  )

  $InSection = $false
  foreach ($Line in [System.IO.File]::ReadLines($Path)) {
    if ($Line -match "^\s*\[(.+)\]\s*$") {
      $InSection = $Matches[1] -eq $Section
      continue
    }
    if ($InSection -and $Line -match "^\s*$([regex]::Escape($Key))\s*=\s*(.+)\s*$") {
      return $Matches[1]
    }
  }
  return $null
}

function Show-WindowsCursor {
  $NativeMethods = "DeskflowCursorRestore.NativeMethods" -as [type]
  if (-not $NativeMethods) {
    Add-Type -Namespace DeskflowCursorRestore -Name NativeMethods -MemberDefinition @"
[System.Runtime.InteropServices.DllImport("user32.dll")]
public static extern int ShowCursor(bool bShow);
"@
    $NativeMethods = "DeskflowCursorRestore.NativeMethods" -as [type]
  }

  for ($Attempt = 0; $Attempt -lt 10; $Attempt++) {
    $DisplayCounter = $NativeMethods::ShowCursor($true)
    if ($DisplayCounter -ge 0) {
      return
    }
  }
}

Write-Host "Stopping Deskflow processes from $BinPath"
$ProcessNames = @("deskflow", "deskflow-core", "deskflow-daemon")
$BinPathFull = [System.IO.Path]::GetFullPath($BinPath).TrimEnd("\", "/")
$ProcessDetails = @{}
Get-CimInstance Win32_Process | Where-Object {
  $ProcessNames -contains [System.IO.Path]::GetFileNameWithoutExtension($_.Name)
} | ForEach-Object {
  $ProcessDetails[[int]$_.ProcessId] = $_
}

Get-Process -Name $ProcessNames -ErrorAction SilentlyContinue | ForEach-Object {
  $Process = $_
  $Details = $ProcessDetails[[int]$Process.Id]
  $Candidates = @()

  try {
    if ($Process.Path) {
      $Candidates += $Process.Path
    }
  } catch {
  }

  if ($Details) {
    if ($Details.ExecutablePath) {
      $Candidates += $Details.ExecutablePath
    }
    if ($Details.CommandLine) {
      $Candidates += $Details.CommandLine
    }
  }

  if ($Process.MainWindowTitle) {
    $Candidates += $Process.MainWindowTitle
  }

  $IsFromBuildBin = $false
  foreach ($Candidate in $Candidates) {
    if ($Candidate.IndexOf($BinPathFull, [StringComparison]::OrdinalIgnoreCase) -ge 0) {
      $IsFromBuildBin = $true
      break
    }
  }

  if ($IsFromBuildBin -or $StopAll) {
    Write-Host "Stopping $($Process.ProcessName) [$($Process.Id)]"
    try {
      Stop-Process -Id $Process.Id -Force
    } catch {
      throw "Failed to stop $($Process.ProcessName) [$($Process.Id)]. Run this script from an elevated PowerShell, or use -StopAll and accept the UAC prompt. $($_.Exception.Message)"
    }
  } elseif ($Candidates.Count -eq 0) {
    Write-Warning "Leaving $($Process.ProcessName) [$($Process.Id)] running because its path is not accessible. Use -StopAll to stop every Deskflow process."
  }
}

Write-Host "Restoring Windows cursor visibility"
Show-WindowsCursor

$Targets = @($BuildTarget)
if (-not $NoDaemon -and $BuildTarget -ne "deskflow-daemon") {
  $Targets += "deskflow-daemon"
}
if (($BuildGui -or -not $NoGuiBuild) -and $BuildTarget -ne "deskflow") {
  $Targets += "deskflow"
}

foreach ($Target in $Targets) {
  Write-Host "Building $Target"
  $Command = "call `"$VsDevCmd`" -arch=x64 -host_arch=x64 && `"$CMakeExe`" --build `"$BuildPath`" --target $Target"
  & cmd.exe /d /s /c $Command
  if ($LASTEXITCODE -ne 0) {
    throw "Build failed for target: $Target"
  }
}

$SettingsDir = Join-Path $env:APPDATA $AppName
$SettingsFile = Join-Path $SettingsDir "$AppName.conf"
$PortableSettingsFile = Join-Path $BinPath "settings/$AppName.conf"
if (Test-Path $PortableSettingsFile) {
  $SettingsFile = $PortableSettingsFile
  $SettingsDir = Split-Path -Parent $SettingsFile
}

New-Item -ItemType Directory -Path $SettingsDir -Force | Out-Null
if (-not (Test-Path $SettingsFile)) {
  New-Item -ItemType File -Path $SettingsFile -Force | Out-Null
}

if ($NoCore) {
  Write-Host "Disabling GUI core auto-start in $SettingsFile"
  Set-AsciiIniValueBytes -Path $SettingsFile -Section "gui" -Key "startCoreWithGui" -Value "false"
} elseif ($DirectCore) {
  Write-Host "Disabling GUI core auto-start in $SettingsFile"
  Set-AsciiIniValueBytes -Path $SettingsFile -Section "gui" -Key "startCoreWithGui" -Value "false"
} else {
  Write-Host "Enabling GUI core auto-start in $SettingsFile"
  Set-AsciiIniValueBytes -Path $SettingsFile -Section "gui" -Key "startCoreWithGui" -Value "true"
}

if (-not $NoDaemon) {
  $DaemonExe = Join-Path $BinPath "deskflow-daemon.exe"
  if (-not (Test-Path $DaemonExe)) {
    throw "deskflow-daemon.exe not found: $DaemonExe"
  }

  Write-Host "Starting foreground daemon: $DaemonExe -f"
  Start-Process -FilePath $DaemonExe -ArgumentList "-f" -WorkingDirectory $BinPath -WindowStyle Hidden
  Start-Sleep -Seconds 1
}

if ($DirectCore -and -not $NoCore) {
  $CoreExe = Join-Path $BinPath "deskflow-core.exe"
  if (-not (Test-Path $CoreExe)) {
    throw "deskflow-core.exe not found: $CoreExe"
  }

  $CoreModeValue = Get-IniValue -Path $SettingsFile -Section "core" -Key "coreMode"
  if ($CoreModeValue -eq "1") {
    $CoreMode = "client"
  } elseif ($CoreModeValue -eq "2") {
    $CoreMode = "server"
  } else {
    throw "core/coreMode must be 1 (client) or 2 (server), got: $CoreModeValue"
  }

  Write-Host "Starting core: $CoreExe $CoreMode --settings $SettingsFile"
  Start-Process -FilePath $CoreExe -ArgumentList @($CoreMode, "--settings", $SettingsFile) -WorkingDirectory $BinPath -WindowStyle Hidden
  Start-Sleep -Seconds 1
}

$DeskflowExe = Join-Path $BinPath "deskflow.exe"
if (-not (Test-Path $DeskflowExe)) {
  throw "deskflow.exe not found: $DeskflowExe"
}

Write-Host "Starting $DeskflowExe"
Start-Process -FilePath $DeskflowExe -WorkingDirectory $BinPath

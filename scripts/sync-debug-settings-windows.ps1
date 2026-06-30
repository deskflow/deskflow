#requires -Version 5.1
<#
.SYNOPSIS
  Ensure debug sessions use the same Deskflow settings as the installed app.
.DESCRIPTION
  Syncs coordination/peers from the server screen layout when missing, matching
  scripts/sync-debug-settings-macos.sh. Sharing settings (client/mouser* and
  server/hidPassthrough* / server/mouserBridge*) live in Deskflow.conf and are
  preserved as-is; configure them in Server Configuration or Client Configuration
  → Sharing.
#>
param(
  [string]$SettingsDir = $(if ($env:DESKFLOW_SETTINGS_DIR) { $env:DESKFLOW_SETTINGS_DIR } else { Join-Path $env:APPDATA 'Deskflow' })
)

$ErrorActionPreference = 'Stop'

$conf = Join-Path $SettingsDir 'Deskflow.conf'
$serverConf = Join-Path $SettingsDir 'deskflow-server.conf'

if (-not (Test-Path $conf)) {
  Write-Host "sync-debug-settings: no settings at $conf (skipping)"
  exit 0
}

function Get-PeersFromServerConf {
  param([string]$Path)
  if (-not (Test-Path $Path)) { return $null }
  $inScreens = $false
  $names = [System.Collections.Generic.List[string]]::new()
  foreach ($line in Get-Content -LiteralPath $Path) {
    if ($line -eq 'section: screens') { $inScreens = $true; continue }
    if ($inScreens -and $line -eq 'end') { break }
    if ($inScreens -and $line -match '^\t([A-Za-z0-9_.-]+):$') {
      $names.Add($Matches[1])
    }
  }
  if ($names.Count -eq 0) { return $null }
  return ($names -join ',')
}

function Get-PeersFromDeskflowConf {
  param([string]$Path)
  $names = [System.Collections.Generic.List[string]]::new()
  foreach ($line in Get-Content -LiteralPath $Path) {
    if ($line -match '^\[screen_([^\]]+)\]$') {
      $names.Add($Matches[1])
    }
  }
  if ($names.Count -eq 0) { return $null }
  return ($names -join ',')
}

function Get-CurrentPeers {
  param([string]$Path)
  $inCoord = $false
  foreach ($line in Get-Content -LiteralPath $Path) {
    if ($line -eq '[coordination]') { $inCoord = $true; continue }
    if ($inCoord -and $line -match '^\[') { break }
    if ($inCoord -and $line -match '^peers=(.*)$') {
      return $Matches[1]
    }
  }
  return $null
}

$existing = Get-CurrentPeers -Path $conf
if ($existing) {
  Write-Host "sync-debug-settings: coordination/peers already set ($existing)"
  exit 0
}

$peers = Get-PeersFromServerConf -Path $serverConf
if (-not $peers) {
  $peers = Get-PeersFromDeskflowConf -Path $conf
}

if (-not $peers) {
  Write-Host "sync-debug-settings: no screen names found in $serverConf or $conf"
  Write-Host '  Configure server screens in the installed app first, or add auto-switch peers manually.'
  exit 0
}

$lines = Get-Content -LiteralPath $conf
$coordIndex = [Array]::IndexOf($lines, '[coordination]')
if ($coordIndex -ge 0) {
  $insertAt = $coordIndex + 1
  $newLines = [System.Collections.Generic.List[string]]::new()
  $newLines.AddRange($lines[0..$coordIndex])
  $newLines.Add('enabled=true')
  $newLines.Add("peers=$peers")
  if ($insertAt -lt $lines.Length) {
    $newLines.AddRange($lines[$insertAt..($lines.Length - 1)])
  }
  $newLines | Set-Content -LiteralPath $conf -Encoding UTF8
} else {
  @(
    ''
    '[coordination]'
    'enabled=true'
    "peers=$peers"
  ) | Add-Content -LiteralPath $conf -Encoding UTF8
}

Write-Host "sync-debug-settings: set coordination/peers=$peers in $conf"

#requires -Version 5.1
<#
.SYNOPSIS
  Stop the Deskflow Windows service and all Deskflow processes.
.DESCRIPTION
  Mirrors the quit step in scripts/install-windows.ps1 so debug sessions do not
  conflict with an installed copy or a running service.
#>
$ErrorActionPreference = 'Stop'

$processNames = @('deskflow', 'deskflow-core', 'deskflow-daemon', 'deskflow-vhid-bridge')

function Get-DeskflowProcesses {
  Get-CimInstance Win32_Process -Filter "Name LIKE 'deskflow%'" -ErrorAction SilentlyContinue
}

Write-Host '== Stopping Deskflow service and all processes =='

$svc = Get-Service -Name Deskflow -ErrorAction SilentlyContinue
if ($svc -and $svc.Status -eq 'Running') {
  Stop-Service -Name Deskflow -Force -ErrorAction SilentlyContinue
  Start-Sleep -Seconds 2
}
if (Get-Service -Name Deskflow -ErrorAction SilentlyContinue) {
  sc.exe stop Deskflow 2>$null | Out-Null
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

  foreach ($name in $processNames) {
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

Write-Host 'Deskflow processes stopped.'

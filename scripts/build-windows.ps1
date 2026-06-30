#requires -Version 5
<#
.SYNOPSIS
  Build (and optionally install) Deskflow on Windows, reading build config from .env.
.DESCRIPTION
  Reads QT_PREFIX and OPENSSL_ROOT_DIR from the repo-root .env (see .env.example),
  configures with CMake + Ninja under an MSVC dev environment, builds the core,
  daemon, and GUI, and -- with -Install -- deploys them to C:\Program Files\Deskflow
  and restarts the Deskflow service.
.EXAMPLE
  pwsh scripts\build-windows.ps1
  pwsh scripts\build-windows.ps1 -Install
#>
param([switch]$Install)
$ErrorActionPreference = "Stop"
$root = Split-Path $PSScriptRoot -Parent

# --- load .env (KEY=VALUE lines; '#' comments ignored) ---
$envFile = Join-Path $root ".env"
if (Test-Path $envFile) {
  Get-Content $envFile | ForEach-Object {
    if ($_ -match '^\s*([A-Za-z_][A-Za-z0-9_]*)\s*=\s*(.*)$') {
      Set-Item -Path "env:$($matches[1])" -Value ($matches[2].Trim())
    }
  }
} else {
  throw "No .env at $envFile -- copy .env.example to .env and fill it in."
}

$qt   = $env:QT_PREFIX;        if (-not $qt)   { throw "QT_PREFIX not set in .env" }
$ossl = $env:OPENSSL_ROOT_DIR; if (-not $ossl) { throw "OPENSSL_ROOT_DIR not set in .env" }

$vcvars = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
if (-not (Test-Path $vcvars)) { throw "vcvars64.bat not found at $vcvars" }
$build = Join-Path $root "build"

Write-Host "Building Deskflow: Qt=$qt OpenSSL=$ossl"
cmd /c "`"$vcvars`" >nul 2>&1 && cmake -S `"$root`" -B `"$build`" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=`"$qt`" -DOPENSSL_ROOT_DIR=`"$ossl`" -DSKIP_BUILD_TESTS=ON && ninja -C `"$build`" deskflow-core deskflow-daemon deskflow"
if ($LASTEXITCODE -ne 0) { throw "build failed (exit $LASTEXITCODE)" }
Write-Host "Build OK -> $build\bin"

if ($Install) {
  Write-Host "Installing to C:\Program Files\Deskflow (single instance) ..."
  Stop-Service Deskflow -Force -ErrorAction SilentlyContinue; Start-Sleep 2
  Get-Process deskflow-core,deskflow-daemon,deskflow -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue; Start-Sleep 2
  foreach ($exe in "deskflow-core.exe","deskflow-daemon.exe","deskflow.exe") {
    Copy-Item (Join-Path "$build\bin" $exe) "C:\Program Files\Deskflow\" -Force -ErrorAction SilentlyContinue
  }
  Start-Service Deskflow -ErrorAction SilentlyContinue
  Write-Host ("Installed; service=" + (Get-Service Deskflow).Status)
}

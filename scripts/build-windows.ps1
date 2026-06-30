#requires -Version 5.1
<#
.SYNOPSIS
  Build (and optionally install) Deskflow on Windows, reading config from .env.
.DESCRIPTION
  Configures with CMake + MSVC, builds deskflow-core, deskflow-daemon, deskflow GUI,
  and deskflow-vhid-bridge. With -Install, delegates to scripts/install-windows.ps1
  for a full quit → cmake --install → service → GUI restart cycle.
.EXAMPLE
  pwsh scripts\build-windows.ps1
  pwsh scripts\build-windows.ps1 -Install
#>
param([switch]$Install)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent

function Import-DeskflowEnv {
  $envFile = Join-Path $root '.env'
  if (-not (Test-Path $envFile)) {
    throw "No .env at $envFile - copy env.example to .env and fill it in."
  }
  Get-Content $envFile | ForEach-Object {
    if ($_ -match '^\s*([A-Za-z_][A-Za-z0-9_]*)\s*=\s*(.*)$' -and $_ -notmatch '^\s*#') {
      Set-Item -Path "env:$($matches[1])" -Value ($matches[2].Trim())
    }
  }
}

function Resolve-QtPath {
  if ($env:DESKFLOW_QT_PATH) { return $env:DESKFLOW_QT_PATH }
  if ($env:QT_PREFIX) { return $env:QT_PREFIX }
  foreach ($c in @(
    'C:\Qt\6.8.3\msvc2022_64', 'C:\Qt\6.7.3\msvc2022_64', 'C:\Qt\6.7.3\msvc2019_64'
  )) {
    if (Test-Path (Join-Path $c 'lib\cmake\Qt6\Qt6Config.cmake')) { return $c }
  }
  $found = Get-ChildItem 'C:\Qt\*\msvc*_64\lib\cmake\Qt6\Qt6Config.cmake' -ErrorAction SilentlyContinue |
    Sort-Object FullName -Descending | Select-Object -First 1
  if ($found) {
    return (Resolve-Path (Join-Path $found.Directory.Parent.Parent.Parent.FullName)).Path
  }
  throw 'Qt 6.7+ not found. Set DESKFLOW_QT_PATH in .env or install via the Qt online installer.'
}

function Resolve-OpenSslRoot {
  if ($env:OPENSSL_ROOT_DIR) { return $env:OPENSSL_ROOT_DIR }
  foreach ($c in @('C:\Program Files\OpenSSL-Win64', 'C:\OpenSSL-Win64')) {
    if (Test-Path (Join-Path $c 'bin\libcrypto-3-x64.dll')) { return $c }
  }
  throw 'OpenSSL not found. Set OPENSSL_ROOT_DIR in .env (e.g. C:\Program Files\OpenSSL-Win64).'
}

Import-DeskflowEnv
$qt = Resolve-QtPath
$ossl = Resolve-OpenSslRoot
$buildDir = if ($env:DESKFLOW_BUILD_DIR) { $env:DESKFLOW_BUILD_DIR } else { 'build' }
$build = if ([System.IO.Path]::IsPathRooted($buildDir)) { $buildDir } else { Join-Path $root $buildDir }

$vcvars = @(
  'C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat',
  'C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat',
  'C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat'
) | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $vcvars) { throw 'vcvars64.bat not found - install Visual Studio Build Tools (C++ workload).' }

Write-Host "Building Deskflow: Qt=$qt OpenSSL=$ossl Build=$build"
$cmakeArgs = @(
  '-S', $root,
  '-B', $build,
  '-DCMAKE_BUILD_TYPE=Release',
  "-DCMAKE_PREFIX_PATH=$qt",
  "-DOPENSSL_ROOT_DIR=$ossl",
  '-DSKIP_BUILD_TESTS=ON',
  '-DBUILD_TESTS=OFF'
)
$buildTargets = 'deskflow-core;deskflow-daemon;deskflow;deskflow-vhid-bridge'
cmd /c "`"$vcvars`" >nul 2>&1 && cmake $($cmakeArgs -join ' ') && cmake --build `"$build`" --config Release --target $buildTargets -j $env:NUMBER_OF_PROCESSORS"
if ($LASTEXITCODE -ne 0) { throw "build failed (exit $LASTEXITCODE)" }
Write-Host "Build OK -> $build"

if ($Install) {
  & (Join-Path $PSScriptRoot 'install-windows.ps1')
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

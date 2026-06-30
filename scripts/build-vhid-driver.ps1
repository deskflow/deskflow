# Build deskflow-vhid.sys (KMDF + VHF) with the Windows Driver Kit.
param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release",
    [ValidateSet("x64")]
    [string]$Platform = "x64"
)

$ErrorActionPreference = "Stop"
$RepoRoot = Split-Path $PSScriptRoot -Parent
$Project = Join-Path $RepoRoot "src\driver\deskflow-vhid\deskflow-vhid.vcxproj"
$OutDir = Join-Path $RepoRoot "build\driver\$Platform\$Configuration"

if (-not (Test-Path $Project)) {
    throw "Driver project not found: $Project"
}

$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) {
    throw "vswhere not found - install Visual Studio with C++ workload"
}

$vsPath = & $vswhere -latest -property installationPath
$msbuild = Join-Path $vsPath "MSBuild\Current\Bin\MSBuild.exe"
if (-not (Test-Path $msbuild)) {
    throw "MSBuild not found under $vsPath"
}

$wdkRoot = "C:\Program Files (x86)\Windows Kits\10\build"
$wdkTargets = Get-ChildItem $wdkRoot -Recurse -Filter "WindowsDriver.Common.targets" -ErrorAction SilentlyContinue | Select-Object -First 1
if (-not $wdkTargets) {
    throw "WDK not found - install Windows Driver Kit from Visual Studio Individual Components"
}

Write-Host "Building deskflow-vhid ($Configuration|$Platform)..."
& $msbuild $Project /p:Configuration=$Configuration /p:Platform=$Platform /restore /v:m
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "Output: $OutDir\deskflow-vhid.sys"
if (Test-Path $OutDir) {
    Copy-Item (Join-Path $RepoRoot "src\driver\deskflow-vhid\deskflow-vhid.inf") $OutDir -Force
    Get-ChildItem $OutDir | Select-Object Name, Length, LastWriteTime
} else {
    Write-Host "Build output directory missing - check MSBuild errors above."
}

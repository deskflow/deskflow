#
# Tests to see if command exists
#
Function Test-CommandExists {
  Param ($command)
  Try {
    if (Get-Command $command) {
      RETURN $true
    }
  }
  Catch {
    RETURN $false
  }
}


#
# Check prerequisites
#
If (-not (Test-CommandExists "cmake" -ErrorAction SilentlyContinue)) {
  Write-Error "Cmake not found. Aborting." -ErrorAction Stop
}


# https://patrickwbarnes.com/articles/using-msbuild-with-powershell/
#
# Find vswhere (installed with recent Visual Studio versions).
#
If ($vsWhere = Get-Command "vswhere.exe" -ErrorAction SilentlyContinue) {
  $vsWhere = $vsWhere.Path
}
ElseIf (Test-Path "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe") {
  $vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
}
Else {
  Write-Error "vswhere not found. Aborting." -ErrorAction Stop
}


#
# Get path to Visual Studio installation using vswhere.
#
$vsPath = &$vsWhere `
  -latest `
  -products * `
  -version "[16.0,17.0)" `
  -requires Microsoft.Component.MSBuild `
  -property installationPath
If ([string]::IsNullOrEmpty("$vsPath")) {
  Write-Error "Failed to find Visual Studio 2019 installation. Aborting." -ErrorAction Stop
}


#
# Make sure the Visual Studio Command Prompt variables are set.
#
If (-not (Test-Path env:LIBPATH)) {
  # Load VC vars
  Push-Location "${vsPath}\VC\Auxiliary\Build"
  Try {
    cmd /c "vcvarsall.bat x64&set" |
    ForEach-Object {
      If ($_ -match "=") {
        $v = $_.split("=")
        Set-Item -Force -Path "ENV:\$($v[0])" -Value "$($v[1])"
      }
    }
  }
  Finally {
    Pop-Location
  }
}


#
# Set undefined environment variables
#
$env:SYNERGY_NO_LEGACY = 1
If (-not (Test-Path env:SYNERGY_BUILD_TYPE)) {
  $env:SYNERGY_BUILD_TYPE = "Release"
}
If (-not (Test-Path env:SYNERGY_BUILD_DIRECTORY)) {
  $env:SYNERGY_BUILD_DIRECTORY = '.\build'
}


#
# Create build folder and make builds
#
New-Item `
  -Path . `
  -Name $env:SYNERGY_BUILD_DIRECTORY `
  -ItemType "directory" `
  -ErrorAction SilentlyContinue | Out-Null
Push-Location $env:SYNERGY_BUILD_DIRECTORY
Try {
  cmake `
    -G "Visual Studio 16 2019" `
    -DCMAKE_BUILD_TYPE=$env:SYNERGY_BUILD_TYPE `
    ..

  msbuild synergy-core.sln `
    /p:Platform="x64" `
    /p:Configuration=$env:SYNERGY_BUILD_TYPE `
    /m `
    /t:synergys `
    /t:synergyc
  Compress-Archive `
    -Path ".\bin\${env:SYNERGY_BUILD_TYPE}\*" `
    -DestinationPath "synergy-core-win-x64.zip"
  Write-Output "::set-output name=location::build\synergy-core-win-x64.zip"
  Write-Output "::set-output name=name::synergy-core-win-x64.zip"
}
Finally {
  Pop-Location
}

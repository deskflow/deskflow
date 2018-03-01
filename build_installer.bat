@echo off
set WIX_ROOT=C:\Program Files (x86)\WiX Toolset v3.11

set savedir=%cd%
cd /d %~dp0

if not exist build\bin\Release goto buildproject

cd build\installer
if ERRORLEVEL 1 goto buildproject

echo Building 64-bit Windows installer...
"%WIX_ROOT%\bin\candle.exe" -nologo -arch x64 -dConfiguration=Release -dPlatform=x64 -ext WixUtilExtension -ext WixFirewallExtension Product.wxs -o Barrier.wixobj
if ERRORLEVEL 1 goto failed
"%WIX_ROOT%\bin\light.exe" -nologo -ext WixUtilExtension -ext WixFirewallExtension -ext WixUIExtension Barrier.wixobj -o bin\Barrier.msi
if ERRORLEVEL 1 goto failed
echo Build completed successfully
goto done

:buildproject
echo To build a 64-bit Windows installer:
echo  - set Q_BUILD_TYPE=Release in build_env.bat
echo  - also set other environmental overrides necessary for your build environment
echo  - run clean_build.bat to build Barrier and verify that it succeeds
echo  - re-run this script to create the installation package
goto done

:failed
echo Build failed

:done
set WIX_ROOT=

cd /d %savedir%
set savedir=

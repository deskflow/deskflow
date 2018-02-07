@echo off

REM defaults - override them by creating a winbuild_env.bat file
set B_BUILD_TYPE=Debug
set B_QT_ROOT=C:\Qt\5.6.3\msvc2015_64
set B_BONJOUR=C:\Program Files\Bonjour SDK

REM  needed by cmake to set bonjour include dir
set BONJOUR_SDK_HOME=%B_BONJOUR%

set savedir=%cd%
cd /d %~dp0

if exist winbuild_env.bat call winbuild_env.bat

rmdir /q /s build
mkdir build
if ERRORLEVEL 1 goto failed
cd build
cmake -G "Visual Studio 15 2017 Win64" -D CMAKE_BUILD_TYPE=%B_BUILD_TYPE% -D CMAKE_PREFIX_PATH="%B_QT_ROOT%" -D DNSSD_LIB="%B_BONJOUR%\Lib\x64\dnssd.lib" ..
if ERRORLEVEL 1 goto failed
echo @msbuild barrier.sln /p:Platform="x64" /p:Configuration=%B_BUILD_TYPE% /m > make.bat
call make.bat
if ERRORLEVEL 1 goto failed
if exist bin\Debug (
    copy %B_QT_ROOT%\bin\Qt5Cored.dll bin\Debug\ > NUL
    copy %B_QT_ROOT%\bin\Qt5Guid.dll bin\Debug\ > NUL
    copy %B_QT_ROOT%\bin\Qt5Networkd.dll bin\Debug\ > NUL
    copy %B_QT_ROOT%\bin\Qt5Widgetsd.dll bin\Debug\ > NUL
    copy %B_QT_ROOT%\bin\Qt5Cored.dll bin\Debug\ > NUL
    copy ..\ext\openssl\windows\x64\bin\* bin\Debug\ > NUL
    copy ..\res\openssl\barrier.conf bin\Debug\ > NUL
) else if exist bin\Release (
    copy %B_QT_ROOT%\bin\Qt5Core.dll bin\Release\ > NUL
    copy %B_QT_ROOT%\bin\Qt5Gui.dll bin\Release\ > NUL
    copy %B_QT_ROOT%\bin\Qt5Network.dll bin\Release\ > NUL
    copy %B_QT_ROOT%\bin\Qt5Widgets.dll bin\Release\ > NUL
    copy %B_QT_ROOT%\bin\Qt5Core.dll bin\Release\ > NUL
    copy ..\ext\openssl\windows\x64\bin\* bin\Release\ > NUL
    copy ..\res\openssl\barrier.conf bin\Release\ > NUL
) else (
    echo Remember to copy supporting binaries and confiuration files!
)

echo Build completed successfully
goto done

:failed
echo Build failed

:done
cd /d %savedir%

set BONJOUR_SDK_HOME=
set B_BUILD_TYPE=
set B_QT_ROOT=
set B_BONJOUR=
set savedir=

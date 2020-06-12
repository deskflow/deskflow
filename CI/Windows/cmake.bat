call "%ENV_BAT%" -arch=x%ARCH%
set BONJOUR_SDK_HOME=%cd%\%BONJOUR_SDK_DIR%
mkdir build%ARCH%
cd build%ARCH%
IF "%Enterprise%"=="true" (
    cmake -G "Visual Studio 16 2019" -A %MSARCH% -V -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=%QT_PATH% -DSYNERGY_ENTERPRISE=ON ..
) else (
    cmake -G "Visual Studio 16 2019" -A %MSARCH% -V -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=%QT_PATH% ..
)
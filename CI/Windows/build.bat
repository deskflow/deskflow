SET VS_INSTALL_PATH=C:\"Program Files (x86)"\"Microsoft Visual Studio"\2019\Community\
call %VS_INSTALL_PATH%Common7\Tools\VsDevCmd.bat
mkdir build
cd build
cmake -G "Visual Studio 15 2017 Win64" -DCMAKE_BUILD_TYPE=Debug ..
msbuild synergy-core.sln /p:Platform="x64" /p:Configuration=Debug /m
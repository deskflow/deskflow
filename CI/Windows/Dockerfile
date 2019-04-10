# escape=`

# Copyright (C) Microsoft Corporation. All rights reserved.
# Licensed under the MIT license. See LICENSE.txt in the project root for license information.

ARG FROM_IMAGE=mcr.microsoft.com/dotnet/framework/sdk:3.5-windowsservercore-ltsc2019
FROM ${FROM_IMAGE}

# Reset the shell.
SHELL ["powershell", "-command"]

ENV TEMP=C:\TEMP\

ENV VS_VERSION=15
ENV QT_VER=5.9.5
ENV QT_VER_S=5.9

ENV QT_INSTALL_DIR=C:\Qt\

# Set up environment to collect install errors.
COPY CI\Windows\ C:\TEMP\

#To reduce the number of layers as windows is massive as it is download, install, and delete the installers in one RUN step
RUN echo \"Downloading build tools (https://aka.ms/vs/${env:VS_VERSION}/release/channel and https://aka.ms/vs/${env:VS_VERSION}/release/vs_buildtools.exe)\"; `
    wget https://aka.ms/vs/${env:VS_VERSION}/release/channel -OutFile ${env:TEMP}VisualStudio.chman; `
    wget https://aka.ms/vs/${env:VS_VERSION}/release/vs_buildtools.exe -OutFile ${env:TEMP}vs_buildtools.exe; `
    echo \"Installing build tools\"; `
    Start-Process ${env:TEMP}vs_buildtools.exe -ArgumentList \"--quiet --wait --norestart --nocache --channelUri ${env:TEMP}VisualStudio.chman --installChannelUri ${env:TEMP}VisualStudio.chman --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended --installPath C:\BuildTools\" -NoNewWindow -Wait; `
    echo \"Downloading QT (Sit back and relax, this is going to take some time)\"; `
    wget https://download.qt.io/official_releases/qt/${env:QT_VER_S}/${env:QT_VER}/qt-opensource-windows-x86-${env:QT_VER}.exe -OutFile ${env:TEMP}qt.exe; `
    echo \"Installing QT\"; `
    Start-Process ${env:TEMP}qt.exe -ArgumentList \"--verbose --script ${env:TEMP}qtifwsilent.qs\" -NoNewWindow -Wait; `
    Remove-Item -Path ${env:TEMP}VisualStudio.chman; `
    Remove-Item -Path ${env:TEMP}vs_buildtools.exe; `
    Remove-Item -Path ${env:TEMP}qt.exe;

CMD ["powershell.exe", "-NoLogo", "-ExecutionPolicy", "Bypass"]
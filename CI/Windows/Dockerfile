# escape=`

# Copyright (C) Microsoft Corporation. All rights reserved.
# Licensed under the MIT license. See LICENSE.txt in the project root for license information.

ARG FROM_IMAGE=mcr.microsoft.com/dotnet/framework/sdk:3.5-windowsservercore-ltsc2019

ARG VS_BUILDTOOLS_VER=15
ARG QT_VER_SHORT=5.9
ARG QT_VER=5.9.5

ARG CHANNEL_URL=https://aka.ms/vs/${VS_BUILDTOOLS_VER}/release/channel
ARG BUILD_TOOLS_WEB=https://aka.ms/vs/${VS_BUILDTOLLS_VER}/release/vs_buildtools.exe
ARG QT_INSTALLER_WEB=https://download.qt.io/official_releases/qt/${QT_VER_SHORT}/${QT_VER}/qt-opensource-windows-x86-${QT_VER}.exe

FROM ${FROM_IMAGE}

# Reset the shell.
SHELL ["cmd", "/S", "/C"]

# Set up environment to collect install errors.
COPY CI\Windows\ C:\TEMP\
ADD https://aka.ms/vscollect.exe C:\TEMP\collect.exe
# Install Node.js LTS
ADD https://nodejs.org/dist/v8.11.3/node-v8.11.3-x64.msi C:\TEMP\node-install.msi
# Download channel for fixed install.
ADD ${CHANNEL_URL} C:\TEMP\VisualStudio.chman
# Download and install Build Tools for Visual Studio
ADD ${BUILD_TOOLS_WEB} C:\TEMP\vs_buildtools.exe
#Download the QT files
ADD ${QT_INSTALLER_WEB} C:\qt.exe

RUN C:\TEMP\InstallVS.cmd

ENTRYPOINT C:\BuildTools\Common7\Tools\VsDevCmd.bat &&
CMD ["powershell.exe", "-NoLogo", "-ExecutionPolicy", "Bypass"]
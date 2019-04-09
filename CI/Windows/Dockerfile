# escape=`

# Copyright (C) Microsoft Corporation. All rights reserved.
# Licensed under the MIT license. See LICENSE.txt in the project root for license information.

ARG FROM_IMAGE=mcr.microsoft.com/dotnet/framework/sdk:3.5-windowsservercore-ltsc2019

FROM ${FROM_IMAGE}

# Reset the shell.
SHELL ["cmd", "/S", "/C"]

# Set up environment to collect install errors.
COPY CI\Windows\ C:\TEMP\
ADD https://aka.ms/vscollect.exe C:\TEMP\collect.exe
# Install Node.js LTS
ADD https://nodejs.org/dist/v8.11.3/node-v8.11.3-x64.msi C:\TEMP\node-install.msi
# Download channel for fixed install.
ADD https://aka.ms/vs/16/release/channel C:\TEMP\VisualStudio.chman
# Download and install Build Tools for Visual Studio
ADD https://aka.ms/vs/16/release/vs_buildtools.exe C:\TEMP\vs_buildtools.exe

#Download the QT files
ADD https://download.qt.io/official_releases/qt/5.9/5.9.5/qt-opensource-windows-x86-5.9.5.exe C:\qt.exe

RUN C:\TEMP\Install.cmd

ENTRYPOINT C:\BuildTools\Common7\Tools\VsDevCmd.bat &&
CMD ["powershell.exe", "-NoLogo", "-ExecutionPolicy", "Bypass"]
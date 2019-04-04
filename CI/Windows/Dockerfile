# escape=`

# Copyright (C) Microsoft Corporation. All rights reserved.
# Licensed under the MIT license. See LICENSE.txt in the project root for license information.

ARG FROM_IMAGE=microsoft/dotnet-framework:3.5-sdk-windowsservercore-1709
FROM ${FROM_IMAGE}

# Reset the shell.
SHELL ["cmd", "/S", "/C"]


# Add the silent installer script for QT
COPY CI\Windows\qtifwsilent.qs C:\qtifwsilent.qs
COPY CI\Windows\Install.cmd C:\TEMP\

# Set up environment to collect install errors.
ADD https://aka.ms/vscollect.exe C:\TEMP\collect.exe

# Download channel for fixed install.
ARG CHANNEL_URL=https://aka.ms/vs/15/release/channel
ADD ${CHANNEL_URL} C:\TEMP\VisualStudio.chman

# Download and install Build Tools for Visual Studio 2017 for native desktop workload.
ADD https://aka.ms/vs/15/release/vs_buildtools.exe C:\TEMP\vs_buildtools.exe

#Download the QT files
ADD https://download.qt.io/official_releases/qt/5.9/5.9.5/qt-opensource-windows-x86-5.9.5.exe C:\qt.exe

# Disable crash dialog for release-mode runtimes
RUN reg add "HKLM\SOFTWARE\Microsoft\Windows\Windows Error Reporting" /v Disabled /t REG_DWORD /d 1 /f;`
    reg add "HKLM\SOFTWARE\Microsoft\Windows\Windows Error Reporting" /v DontShowUI /t REG_DWORD /d 1 /f

#Install MSBuild tools
RUN C:\TEMP\Install.cmd C:\TEMP\vs_buildtools.exe --quiet --wait --norestart --nocache `
    --channelUri C:\TEMP\VisualStudio.chman `
    --installChannelUri C:\TEMP\VisualStudio.chman `
    --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended`
    --installPath C:\BuildTools

#Install QT
RUN powershell -command "$Env:QT_INSTALL_DIR = 'C:\\Qt'; Start-Process C:\qt.exe -ArgumentList '--verbose --script C:\qtifwsilent.qs' -NoNewWindow -Wait"; `
    del C:\TEMP\node-install.msi; del C:\TEMP\vs_buildtools.exe; del C:\qt.exe

# Use developer command prompt and start PowerShell if no other command specified.
ENTRYPOINT C:\BuildTools\Common7\Tools\VsDevCmd.bat &&
CMD ["powershell.exe", "-NoLogo", "-ExecutionPolicy", "Bypass"]
# escape=`

# Copyright (C) Microsoft Corporation. All rights reserved.
# Licensed under the MIT license. See LICENSE.txt in the project root for license information.

ARG FROM_IMAGE=microsoft/dotnet-framework:3.5-sdk-windowsservercore-1709
FROM ${FROM_IMAGE}

# Reset the shell.
SHELL ["cmd", "/S", "/C"]

# Set up environment to collect install errors.
COPY Install.cmd C:\TEMP\
ADD https://aka.ms/vscollect.exe C:\TEMP\collect.exe

# Install Node.js LTS
ADD https://nodejs.org/dist/v8.11.3/node-v8.11.3-x64.msi C:\TEMP\node-install.msi
RUN start /wait msiexec.exe /i C:\TEMP\node-install.msi /l*vx "%TEMP%\MSI-node-install.log" /qn ADDLOCAL=ALL

# Download channel for fixed install.
ARG CHANNEL_URL=https://aka.ms/vs/15/release/channel
ADD ${CHANNEL_URL} C:\TEMP\VisualStudio.chman

# Download and install Build Tools for Visual Studio 2017 for native desktop workload.
ADD https://aka.ms/vs/15/release/vs_buildtools.exe C:\TEMP\vs_buildtools.exe

# Disable crash dialog for release-mode runtimes
RUN reg add "HKLM\SOFTWARE\Microsoft\Windows\Windows Error Reporting" /v Disabled /t REG_DWORD /d 1 /f
RUN reg add "HKLM\SOFTWARE\Microsoft\Windows\Windows Error Reporting" /v DontShowUI /t REG_DWORD /d 1 /f

# Add the silent installer script for QT
COPY qtifwsilent.qs C:\qtifwsilent.qs

#Install MSBuild tools
RUN C:\TEMP\Install.cmd C:\TEMP\vs_buildtools.exe --quiet --wait --norestart --nocache `
    --channelUri C:\TEMP\VisualStudio.chman `
    --installChannelUri C:\TEMP\VisualStudio.chman `
    --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended`
    --installPath C:\BuildTools

#Install QT
RUN powershell -NoProfile -ExecutionPolicy Bypass -Command $ErrorActionPreference = 'Stop'; `
    $Wc = New-Object System.Net.WebClient ; `
    $Wc.DownloadFile('https://download.qt.io/official_releases/qt/5.9/5.9.5/qt-opensource-windows-x86-5.9.5.exe', 'C:\qt.exe') ; `
    Echo 'Downloaded qt-opensource-windows-x86-5.9.5.exe' ; `
    $Env:QT_INSTALL_DIR = 'C:\\Qt' ; `
    Start-Process C:\qt.exe -ArgumentList '--verbose --script C:/qtifwsilent.qs' -NoNewWindow -Wait ; `
    Remove-Item C:\qt.exe -Force ; `
    Remove-Item C:\qtifwsilent.qs -Force
ENV QTDIR C:\\Qt\\5.9.5\\msvc2015
ENV QTDIR64 C:\\Qt\\5.9.5\\msvc2015_64
RUN dir "%QTDIR%" && dir "%QTDIR64%" && dir "%QTDIR%\bin\Qt5Script.dll" && dir "%QTDIR64%\bin\Qt5Script.dll"

# Use developer command prompt and start PowerShell if no other command specified.
ENTRYPOINT C:\BuildTools\Common7\Tools\VsDevCmd.bat &&
CMD ["powershell.exe", "-NoLogo", "-ExecutionPolicy", "Bypass"]
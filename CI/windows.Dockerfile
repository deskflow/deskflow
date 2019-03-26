# escape=`

# Use the latest Windows Server Core image with .NET Framework 4.7.1.
FROM buildtools2017native:latest

# Restore the default Windows shell for correct batch processing below.
SHELL ["cmd", "/S", "/C"]

# Disable crash dialog for release-mode runtimes
RUN reg add "HKLM\SOFTWARE\Microsoft\Windows\Windows Error Reporting" /v Disabled /t REG_DWORD /d 1 /f
RUN reg add "HKLM\SOFTWARE\Microsoft\Windows\Windows Error Reporting" /v DontShowUI /t REG_DWORD /d 1 /f

# Add the silent installer script
COPY qtifwsilent.qs C:\qtifwsilent.qs

# Install QT
RUN powershell -NoProfile -ExecutionPolicy Bypass -Command $ErrorActionPreference = 'Stop'; \
    $Wc = New-Object System.Net.WebClient ; \
    $Wc.DownloadFile('https://download.qt.io/official_releases/qt/5.9/5.9.5/qt-opensource-windows-x86-5.9.5.exe', 'C:\qt.exe') ; \
    Echo 'Downloaded qt-opensource-windows-x86-5.9.5.exe' ; \
    $Env:QT_INSTALL_DIR = 'C:\\Qt' ; \
    Start-Process C:\qt.exe -ArgumentList '--verbose --script C:/qtifwsilent.qs' -NoNewWindow -Wait ; \
    Remove-Item C:\qt.exe -Force ; \
    Remove-Item C:\qtifwsilent.qs -Force
ENV QTDIR C:\\Qt\\5.9.5\\msvc2015
ENV QTDIR64 C:\\Qt\\5.9.5\\msvc2015_64
RUN dir "%QTDIR%" && dir "%QTDIR64%" && dir "%QTDIR%\bin\Qt5Script.dll" && dir "%QTDIR64%\bin\Qt5Script.dll"

# Start developer command prompt with any other commands specified.
ENTRYPOINT C:\BuildTools\Common7\Tools\VsDevCmd.bat &&

# Default to PowerShell if no other command specified.
CMD ["powershell.exe", "-NoLogo", "-ExecutionPolicy", "Bypass"]
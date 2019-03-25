# escape=`

# Use the latest Windows Server Core image with .NET Framework 4.7.1.
FROM buildtools2017native:latest

# Restore the default Windows shell for correct batch processing below.
SHELL ["cmd", "/S", "/C"]

# Download the QT libraries
ADD http://download.qt-project.org/official_releases/qt/5.9/5.9.5/qt-opensource-windows-x86-5.9.5.exe C:\TEMP\qt-opensource-windows-x86-5.9.5.exe

# Install QT
RUN C:\TEMP\qt-opensource-windows-x86-5.9.5.exe 

# Start developer command prompt with any other commands specified.
ENTRYPOINT C:\BuildTools\Common7\Tools\VsDevCmd.bat &&

# Default to PowerShell if no other command specified.
CMD ["powershell.exe", "-NoLogo", "-ExecutionPolicy", "Bypass"]
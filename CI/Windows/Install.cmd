start /wait msiexec.exe /i C:\TEMP\node-install.msi /l*vx "%TEMP%\MSI-node-install.log" /qn ADDLOCAL=ALL

reg add "HKLM\SOFTWARE\Microsoft\Windows\Windows Error Reporting" /v Disabled /t REG_DWORD /d 1 /f
reg add "HKLM\SOFTWARE\Microsoft\Windows\Windows Error Reporting" /v DontShowUI /t REG_DWORD /d 1 /f

C:\TEMP\vs_buildtools.exe --quiet --wait --norestart --nocache --channelUri C:\TEMP\VisualStudio.chman --installChannelUri C:\TEMP\VisualStudio.chman --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended --installPath C:\BuildTools

powershell -command "$Env:QT_INSTALL_DIR = 'C:\\Qt'; Start-Process C:\TEMP\qt.exe -ArgumentList '--verbose --script C:\TEMP\qtifwsilent.qs' -NoNewWindow -Wait"

del C:\TEMP\qt.exe
del C:\TEMP\vs_buildtools.exe
del C:\TEMP\node-install.msi
start /wait msiexec.exe /i C:\TEMP\node-install.msi /l*vx "%TEMP%\MSI-node-install.log" /qn ADDLOCAL=ALL

reg add "HKLM\SOFTWARE\Microsoft\Windows\Windows Error Reporting" /v Disabled /t REG_DWORD /d 1 /f
reg add "HKLM\SOFTWARE\Microsoft\Windows\Windows Error Reporting" /v DontShowUI /t REG_DWORD /d 1 /f

C:\TEMP\InstallVS.cmd C:\TEMP\vs_buildtools.exe --quiet --wait --norestart --nocache `
    --channelUri C:\TEMP\VisualStudio.chman `
    --installChannelUri C:\TEMP\VisualStudio.chman `
    --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended`
    --installPath C:\BuildTools

powershell -command "$Env:QT_INSTALL_DIR = 'C:\\Qt'; Start-Process C:\qt.exe -ArgumentList '--verbose --script C:\qtifwsilent.qs' -NoNewWindow -Wait"

del C:\TEMP\node-install.msi
del C:\TEMP\vs_buildtools.exe
del C:\qt.exe
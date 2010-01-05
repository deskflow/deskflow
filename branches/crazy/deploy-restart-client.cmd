@echo off
xcopy /DHRY *.exe \\192.168.1.100\c$\synergy\
xcopy /DHRY *.dll \\192.168.1.100\c$\synergy\
xcopy /DHRY *.cmd \\192.168.1.100\c$\synergy\
xcopy /DHRY *.manifest \\192.168.1.100\c$\synergy\


pskill \\192.168.1.100 synergyc 2>NUL
::psexec \\192.168.1.100 -w "c:\\synergy" C:\synergy\synergyc.exe




@echo off
xcopy /Y bin\Debug\*.exe \\synergy-client\synergy\
xcopy /Y bin\Debug\*.dll \\synergy-client\synergy\
xcopy /Y restart.cmd \\synergy-client\synergy\

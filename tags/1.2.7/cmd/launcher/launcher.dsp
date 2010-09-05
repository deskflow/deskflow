# Microsoft Developer Studio Project File - Name="launcher" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=launcher - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "launcher.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "launcher.mak" CFG="launcher - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "launcher - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "launcher - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "launcher - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\build"
# PROP Intermediate_Dir "..\..\gen\build"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MT /W4 /GX /O2 /I "..\..\lib\common" /I "..\..\lib\arch" /I "..\..\lib\base" /I "..\..\lib\mt" /I "..\..\lib\io" /I "..\..\lib\net" /I "..\..\lib\synergy" /I "..\..\lib\platform" /I "..\..\lib\server" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /Fd"..\..\gen\build\launcher.pdb" /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386 /out:"..\..\build\synergy.exe"
# SUBTRACT LINK32 /map

!ELSEIF  "$(CFG)" == "launcher - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\debug"
# PROP Intermediate_Dir "..\..\gen\debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W4 /Gm /GX /ZI /Od /I "..\..\lib\common" /I "..\..\lib\arch" /I "..\..\lib\base" /I "..\..\lib\mt" /I "..\..\lib\io" /I "..\..\lib\net" /I "..\..\lib\synergy" /I "..\..\lib\platform" /I "..\..\lib\server" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /Fd"..\..\gen\debug\launcher.pdb" /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /out:"..\..\debug\synergy.exe" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "launcher - Win32 Release"
# Name "launcher - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\CAddScreen.cpp
# End Source File
# Begin Source File

SOURCE=.\CAdvancedOptions.cpp
# End Source File
# Begin Source File

SOURCE=.\CAutoStart.cpp
# End Source File
# Begin Source File

SOURCE=.\CGlobalOptions.cpp
# End Source File
# Begin Source File

SOURCE=.\CScreensLinks.cpp
# End Source File
# Begin Source File

SOURCE=.\launcher.cpp
# End Source File
# Begin Source File

SOURCE=.\launcher.rc
# End Source File
# Begin Source File

SOURCE=.\LaunchUtil.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\CAddScreen.h
# End Source File
# Begin Source File

SOURCE=.\CAdvancedOptions.h
# End Source File
# Begin Source File

SOURCE=.\CAutoStart.h
# End Source File
# Begin Source File

SOURCE=.\CGlobalOptions.h
# End Source File
# Begin Source File

SOURCE=.\CScreensLinks.h
# End Source File
# Begin Source File

SOURCE=.\LaunchUtil.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\synergy.ico
# End Source File
# End Group
# End Target
# End Project

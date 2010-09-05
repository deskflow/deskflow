# Microsoft Developer Studio Project File - Name="libsynergy" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=libsynergy - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libsynergy.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libsynergy.mak" CFG="libsynergy - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libsynergy - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libsynergy - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libsynergy - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\gen\build"
# PROP Intermediate_Dir "..\..\gen\build"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MT /W4 /GX /O2 /I "..\common" /I "..\arch" /I "..\base" /I "..\io" /I "..\mt" /I "..\net" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /Fd"..\..\gen\build\libsynergy.pdb" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "libsynergy - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\gen\debug"
# PROP Intermediate_Dir "..\..\gen\debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W4 /Gm /GX /ZI /Od /I "..\common" /I "..\arch" /I "..\base" /I "..\io" /I "..\mt" /I "..\net" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /Fd"..\..\gen\debug\libsynergy.pdb" /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "libsynergy - Win32 Release"
# Name "libsynergy - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\CClipboard.cpp
# End Source File
# Begin Source File

SOURCE=.\CKeyMap.cpp
# End Source File
# Begin Source File

SOURCE=.\CKeyState.cpp
# End Source File
# Begin Source File

SOURCE=.\CPacketStreamFilter.cpp
# End Source File
# Begin Source File

SOURCE=.\CPlatformScreen.cpp
# End Source File
# Begin Source File

SOURCE=.\CProtocolUtil.cpp
# End Source File
# Begin Source File

SOURCE=.\CScreen.cpp
# End Source File
# Begin Source File

SOURCE=.\IClipboard.cpp
# End Source File
# Begin Source File

SOURCE=.\IKeyState.cpp
# End Source File
# Begin Source File

SOURCE=.\IPrimaryScreen.cpp
# End Source File
# Begin Source File

SOURCE=.\IScreen.cpp
# End Source File
# Begin Source File

SOURCE=.\ProtocolTypes.cpp
# End Source File
# Begin Source File

SOURCE=.\XScreen.cpp
# End Source File
# Begin Source File

SOURCE=.\XSynergy.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\CClipboard.h
# End Source File
# Begin Source File

SOURCE=.\CKeyMap.h
# End Source File
# Begin Source File

SOURCE=.\CKeyState.h
# End Source File
# Begin Source File

SOURCE=.\ClipboardTypes.h
# End Source File
# Begin Source File

SOURCE=.\CPacketStreamFilter.h
# End Source File
# Begin Source File

SOURCE=.\CPlatformScreen.h
# End Source File
# Begin Source File

SOURCE=.\CProtocolUtil.h
# End Source File
# Begin Source File

SOURCE=.\CScreen.h
# End Source File
# Begin Source File

SOURCE=.\IClient.h
# End Source File
# Begin Source File

SOURCE=.\IClipboard.h
# End Source File
# Begin Source File

SOURCE=.\IKeyState.h
# End Source File
# Begin Source File

SOURCE=.\IPlatformScreen.h
# End Source File
# Begin Source File

SOURCE=.\IPrimaryScreen.h
# End Source File
# Begin Source File

SOURCE=.\IScreen.h
# End Source File
# Begin Source File

SOURCE=.\IScreenSaver.h
# End Source File
# Begin Source File

SOURCE=.\ISecondaryScreen.h
# End Source File
# Begin Source File

SOURCE=.\KeyTypes.h
# End Source File
# Begin Source File

SOURCE=.\MouseTypes.h
# End Source File
# Begin Source File

SOURCE=.\OptionTypes.h
# End Source File
# Begin Source File

SOURCE=.\ProtocolTypes.h
# End Source File
# Begin Source File

SOURCE=.\SpecialKeyNameMap.h
# End Source File
# Begin Source File

SOURCE=.\XScreen.h
# End Source File
# Begin Source File

SOURCE=.\XSynergy.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project

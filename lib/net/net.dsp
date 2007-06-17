# Microsoft Developer Studio Project File - Name="net" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=net - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "net.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "net.mak" CFG="net - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "net - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "net - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "net - Win32 Release"

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
# ADD CPP /nologo /MT /W4 /GR /GX /O2 /I "..\common" /I "..\arch" /I "..\base" /I "..\io" /I "..\mt" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /Fd"..\..\gen\build\net.pdb" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "net - Win32 Debug"

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
# ADD CPP /nologo /MTd /W4 /Gm /GR /GX /ZI /Od /I "..\common" /I "..\arch" /I "..\base" /I "..\io" /I "..\mt" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /Fd"..\..\gen\debug\net.pdb" /FD /GZ /c
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

# Name "net - Win32 Release"
# Name "net - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\CNetworkAddress.cpp
# End Source File
# Begin Source File

SOURCE=.\CSocketMultiplexer.cpp
# End Source File
# Begin Source File

SOURCE=.\CTCPListenSocket.cpp
# End Source File
# Begin Source File

SOURCE=.\CTCPSocket.cpp
# End Source File
# Begin Source File

SOURCE=.\CTCPSocketFactory.cpp
# End Source File
# Begin Source File

SOURCE=.\IDataSocket.cpp
# End Source File
# Begin Source File

SOURCE=.\IListenSocket.cpp
# End Source File
# Begin Source File

SOURCE=.\ISocket.cpp
# End Source File
# Begin Source File

SOURCE=.\XSocket.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\CNetworkAddress.h
# End Source File
# Begin Source File

SOURCE=.\CSocketMultiplexer.h
# End Source File
# Begin Source File

SOURCE=.\CTCPListenSocket.h
# End Source File
# Begin Source File

SOURCE=.\CTCPSocket.h
# End Source File
# Begin Source File

SOURCE=.\CTCPSocketFactory.h
# End Source File
# Begin Source File

SOURCE=.\IDataSocket.h
# End Source File
# Begin Source File

SOURCE=.\IListenSocket.h
# End Source File
# Begin Source File

SOURCE=.\ISocket.h
# End Source File
# Begin Source File

SOURCE=.\ISocketFactory.h
# End Source File
# Begin Source File

SOURCE=.\ISocketMultiplexerJob.h
# End Source File
# Begin Source File

SOURCE=.\TSocketMultiplexerMethodJob.h
# End Source File
# Begin Source File

SOURCE=.\XSocket.h
# End Source File
# End Group
# End Target
# End Project

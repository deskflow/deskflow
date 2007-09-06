# synergy -- mouse and keyboard sharing utility
# Copyright (C) 2007 Chris Schoeneman
# 
# This package is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# found in the file COPYING that should have accompanied this file.
# 
# This package is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

BIN_LAUNCHER_SRC = cmd\launcher
BIN_LAUNCHER_DST = $(BUILD_DST)\$(BIN_LAUNCHER_SRC)
BIN_LAUNCHER_EXE = "$(BUILD_DST)\synergy.exe"
BIN_LAUNCHER_CPP =					\
	"CAddScreen.cpp"				\
	"CAdvancedOptions.cpp"			\
	"CAutoStart.cpp"				\
	"CGlobalOptions.cpp"			\
	"CHotkeyOptions.cpp"			\
	"CInfo.cpp"						\
	"CScreensLinks.cpp"				\
	"LaunchUtil.cpp"				\
	"launcher.cpp"					\
	$(NULL)
BIN_LAUNCHER_OBJ =								\
	"$(BIN_LAUNCHER_DST)\CAddScreen.obj"		\
	"$(BIN_LAUNCHER_DST)\CAdvancedOptions.obj"	\
	"$(BIN_LAUNCHER_DST)\CAutoStart.obj"		\
	"$(BIN_LAUNCHER_DST)\CGlobalOptions.obj"	\
	"$(BIN_LAUNCHER_DST)\CHotkeyOptions.obj"	\
	"$(BIN_LAUNCHER_DST)\CInfo.obj"				\
	"$(BIN_LAUNCHER_DST)\CScreensLinks.obj"		\
	"$(BIN_LAUNCHER_DST)\LaunchUtil.obj"		\
	"$(BIN_LAUNCHER_DST)\launcher.obj"			\
	$(NULL)
BIN_LAUNCHER_RC = "$(BIN_LAUNCHER_SRC)\launcher.rc"
BIN_LAUNCHER_RES = "$(BIN_LAUNCHER_DST)\launcher.res"
BIN_LAUNCHER_INC =					\
	/I"lib\common"					\
	/I"lib\arch"					\
	/I"lib\base"					\
	/I"lib\mt"						\
	/I"lib\io"						\
	/I"lib\net"						\
	/I"lib\synergy"					\
	/I"lib\platform"				\
	/I"lib\server"					\
	$(NULL)
BIN_LAUNCHER_LIB =					\
	$(LIB_SERVER_LIB)				\
	$(LIB_PLATFORM_LIB)				\
	$(LIB_SYNERGY_LIB)				\
	$(LIB_NET_LIB)					\
	$(LIB_IO_LIB)					\
	$(LIB_MT_LIB)					\
	$(LIB_BASE_LIB)					\
	$(LIB_ARCH_LIB)					\
	$(LIB_COMMON_LIB)				\
	$(NULL)

CPP_FILES = $(CPP_FILES) $(BIN_LAUNCHER_CPP)
OBJ_FILES = $(OBJ_FILES) $(BIN_LAUNCHER_OBJ)
PROGRAMS  = $(PROGRAMS)  $(BIN_LAUNCHER_EXE)

# Need shell functions.
guilibs = $(guilibs) shell32.lib

# Dependency rules
$(BIN_LAUNCHER_OBJ): $(AUTODEP)
!if EXIST($(BIN_LAUNCHER_DST)\deps.mak)
!include $(BIN_LAUNCHER_DST)\deps.mak
!endif

# Build rules.  Use batch-mode rules if possible.
!if DEFINED(_NMAKE_VER)
{$(BIN_LAUNCHER_SRC)\}.cpp{$(BIN_LAUNCHER_DST)\}.obj::
!else
{$(BIN_LAUNCHER_SRC)\}.cpp{$(BIN_LAUNCHER_DST)\}.obj:
!endif
	@$(ECHO) Compile in $(BIN_LAUNCHER_SRC)
	-@$(MKDIR) $(BIN_LAUNCHER_DST) 2>NUL:
	$(cpp) $(cppdebug) $(cppflags) $(cppvarsmt) /showIncludes \
		$(BIN_LAUNCHER_INC) \
		/Fo$(BIN_LAUNCHER_DST)\ \
		/Fd$(BIN_LAUNCHER_DST)\src.pdb \
		$< | $(AUTODEP) $(BIN_LAUNCHER_SRC) $(BIN_LAUNCHER_DST)
$(BIN_LAUNCHER_RES): $(BIN_LAUNCHER_RC)
	@$(ECHO) Compile $(**F)
	-@$(MKDIR) $(BIN_LAUNCHER_DST) 2>NUL:
	$(rc) $(rcflags) $(rcvars) \
		/fo$@ \
		$**
$(BIN_LAUNCHER_EXE): $(BIN_LAUNCHER_OBJ) $(BIN_LAUNCHER_RES) $(BIN_LAUNCHER_LIB)
	@$(ECHO) Link $(@F)
	$(link) $(ldebug) $(guilflags) $(guilibsmt) \
		/out:$@ \
		$**
	$(AUTODEP) $(BIN_LAUNCHER_SRC) $(BIN_LAUNCHER_DST) \
		$(BIN_LAUNCHER_OBJ:.obj=.d)

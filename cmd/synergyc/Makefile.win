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

BIN_SYNERGYC_SRC = cmd\synergyc
BIN_SYNERGYC_DST = $(BUILD_DST)\$(BIN_SYNERGYC_SRC)
BIN_SYNERGYC_EXE = "$(BUILD_DST)\synergyc.exe"
BIN_SYNERGYC_CPP =							\
	"CClientTaskBarReceiver.cpp"			\
	"CMSWindowsClientTaskBarReceiver.cpp"	\
	"synergyc.cpp"							\
	$(NULL)
BIN_SYNERGYC_OBJ =												\
	"$(BIN_SYNERGYC_DST)\CClientTaskBarReceiver.obj"			\
	"$(BIN_SYNERGYC_DST)\CMSWindowsClientTaskBarReceiver.obj"	\
	"$(BIN_SYNERGYC_DST)\synergyc.obj"							\
	$(NULL)
BIN_SYNERGYC_RC = "$(BIN_SYNERGYC_SRC)\synergyc.rc"
BIN_SYNERGYC_RES = "$(BIN_SYNERGYC_DST)\synergyc.res"
BIN_SYNERGYC_INC =					\
	/I"lib\common"					\
	/I"lib\arch"					\
	/I"lib\base"					\
	/I"lib\mt"						\
	/I"lib\io"						\
	/I"lib\net"						\
	/I"lib\synergy"					\
	/I"lib\platform"				\
	/I"lib\client"					\
	$(NULL)
BIN_SYNERGYC_LIB =					\
	$(LIB_CLIENT_LIB)				\
	$(LIB_PLATFORM_LIB)				\
	$(LIB_SYNERGY_LIB)				\
	$(LIB_NET_LIB)					\
	$(LIB_IO_LIB)					\
	$(LIB_MT_LIB)					\
	$(LIB_BASE_LIB)					\
	$(LIB_ARCH_LIB)					\
	$(LIB_COMMON_LIB)				\
	$(NULL)

CPP_FILES = $(CPP_FILES) $(BIN_SYNERGYC_CPP)
OBJ_FILES = $(OBJ_FILES) $(BIN_SYNERGYC_OBJ)
PROGRAMS  = $(PROGRAMS)  $(BIN_SYNERGYC_EXE)

# Need shell functions.
guilibs = $(guilibs) shell32.lib

# Dependency rules
$(BIN_SYNERGYC_OBJ): $(AUTODEP)
!if EXIST($(BIN_SYNERGYC_DST)\deps.mak)
!include $(BIN_SYNERGYC_DST)\deps.mak
!endif

# Build rules.  Use batch-mode rules if possible.
!if DEFINED(_NMAKE_VER)
{$(BIN_SYNERGYC_SRC)\}.cpp{$(BIN_SYNERGYC_DST)\}.obj::
!else
{$(BIN_SYNERGYC_SRC)\}.cpp{$(BIN_SYNERGYC_DST)\}.obj:
!endif
	@$(ECHO) Compile in $(BIN_SYNERGYC_SRC)
	-@$(MKDIR) $(BIN_SYNERGYC_DST) 2>NUL:
	$(cpp) $(cppdebug) $(cppflags) $(cppvarsmt) /showIncludes \
		$(BIN_SYNERGYC_INC) \
		/Fo$(BIN_SYNERGYC_DST)\ \
		/Fd$(BIN_SYNERGYC_DST)\src.pdb \
		$< | $(AUTODEP) $(BIN_SYNERGYC_SRC) $(BIN_SYNERGYC_DST)
$(BIN_SYNERGYC_RES): $(BIN_SYNERGYC_RC)
	@$(ECHO) Compile $(**F)
	-@$(MKDIR) $(BIN_SYNERGYC_DST) 2>NUL:
	$(rc) $(rcflags) $(rcvars) \
		/fo$@ \
		$**
$(BIN_SYNERGYC_EXE): $(BIN_SYNERGYC_OBJ) $(BIN_SYNERGYC_RES) $(BIN_SYNERGYC_LIB)
	@$(ECHO) Link $(@F)
	$(link) $(ldebug) $(guilflags) $(guilibsmt) \
		/out:$@ \
		$**
	$(AUTODEP) $(BIN_SYNERGYC_SRC) $(BIN_SYNERGYC_DST) \
		$(BIN_SYNERGYC_OBJ:.obj=.d)

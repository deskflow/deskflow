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

LIB_SYNERGY_SRC = lib\synergy
LIB_SYNERGY_DST = $(BUILD_DST)\$(LIB_SYNERGY_SRC)
LIB_SYNERGY_LIB = "$(LIB_SYNERGY_DST)\libsynergy.lib"
LIB_SYNERGY_CPP =					\
	"CClipboard.cpp"				\
	"CKeyMap.cpp"					\
	"CKeyState.cpp"					\
	"CPacketStreamFilter.cpp"		\
	"CPlatformScreen.cpp"			\
	"CProtocolUtil.cpp"				\
	"CScreen.cpp"					\
	"IClipboard.cpp"				\
	"IKeyState.cpp"					\
	"IPrimaryScreen.cpp"			\
	"IScreen.cpp"					\
	"KeyTypes.cpp"					\
	"ProtocolTypes.cpp"				\
	"XScreen.cpp"					\
	"XSynergy.cpp"					\
	$(NULL)
LIB_SYNERGY_OBJ =									\
	"$(LIB_SYNERGY_DST)\CClipboard.obj"				\
	"$(LIB_SYNERGY_DST)\CKeyMap.obj"				\
	"$(LIB_SYNERGY_DST)\CKeyState.obj"				\
	"$(LIB_SYNERGY_DST)\CPacketStreamFilter.obj"	\
	"$(LIB_SYNERGY_DST)\CPlatformScreen.obj"		\
	"$(LIB_SYNERGY_DST)\CProtocolUtil.obj"			\
	"$(LIB_SYNERGY_DST)\CScreen.obj"				\
	"$(LIB_SYNERGY_DST)\IClipboard.obj"				\
	"$(LIB_SYNERGY_DST)\IKeyState.obj"				\
	"$(LIB_SYNERGY_DST)\IPrimaryScreen.obj"			\
	"$(LIB_SYNERGY_DST)\IScreen.obj"				\
	"$(LIB_SYNERGY_DST)\KeyTypes.obj"				\
	"$(LIB_SYNERGY_DST)\ProtocolTypes.obj"			\
	"$(LIB_SYNERGY_DST)\XScreen.obj"				\
	"$(LIB_SYNERGY_DST)\XSynergy.obj"				\
	$(NULL)
LIB_SYNERGY_INC =					\
	/I"lib\common"					\
	/I"lib\arch"					\
	/I"lib\base"					\
	/I"lib\mt"						\
	/I"lib\io"						\
	/I"lib\net"						\
	$(NULL)

CPP_FILES = $(CPP_FILES) $(LIB_SYNERGY_CPP)
OBJ_FILES = $(OBJ_FILES) $(LIB_SYNERGY_OBJ)
LIB_FILES = $(LIB_FILES) $(LIB_SYNERGY_LIB)

# Dependency rules
$(LIB_SYNERGY_OBJ): $(AUTODEP)
!if EXIST($(LIB_SYNERGY_DST)\deps.mak)
!include $(LIB_SYNERGY_DST)\deps.mak
!endif

# Build rules.  Use batch-mode rules if possible.
!if DEFINED(_NMAKE_VER)
{$(LIB_SYNERGY_SRC)\}.cpp{$(LIB_SYNERGY_DST)\}.obj::
!else
{$(LIB_SYNERGY_SRC)\}.cpp{$(LIB_SYNERGY_DST)\}.obj:
!endif
	@$(ECHO) Compile in $(LIB_SYNERGY_SRC)
	-@$(MKDIR) $(LIB_SYNERGY_DST) 2>NUL:
	$(cpp) $(cppdebug) $(cppflags) $(cppvarsmt) /showIncludes \
		$(LIB_SYNERGY_INC) \
		/Fo$(LIB_SYNERGY_DST)\ \
		/Fd$(LIB_SYNERGY_LIB:.lib=.pdb) \
		$< | $(AUTODEP) $(LIB_SYNERGY_SRC) $(LIB_SYNERGY_DST)
$(LIB_SYNERGY_LIB): $(LIB_SYNERGY_OBJ)
	@$(ECHO) Link $(@F)
	$(implib) $(ildebug) $(ilflags) \
		/out:$@ \
		$**
	$(AUTODEP) $(LIB_SYNERGY_SRC) $(LIB_SYNERGY_DST) $(**:.obj=.d)

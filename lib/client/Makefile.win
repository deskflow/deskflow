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

LIB_CLIENT_SRC = lib\client
LIB_CLIENT_DST = $(BUILD_DST)\$(LIB_CLIENT_SRC)
LIB_CLIENT_LIB = "$(LIB_CLIENT_DST)\client.lib"
LIB_CLIENT_CPP =					\
	"CClient.cpp"					\
	"CServerProxy.cpp"				\
	$(NULL)
LIB_CLIENT_OBJ =							\
	"$(LIB_CLIENT_DST)\CClient.obj"			\
	"$(LIB_CLIENT_DST)\CServerProxy.obj"	\
	$(NULL)
LIB_CLIENT_INC =					\
	/I"lib\common"					\
	/I"lib\arch"					\
	/I"lib\base"					\
	/I"lib\mt"						\
	/I"lib\io"						\
	/I"lib\net"						\
	/I"lib\synergy"					\
	/I"lib\platform"				\
	$(NULL)

CPP_FILES = $(CPP_FILES) $(LIB_CLIENT_CPP)
OBJ_FILES = $(OBJ_FILES) $(LIB_CLIENT_OBJ)
LIB_FILES = $(LIB_FILES) $(LIB_CLIENT_LIB)

# Dependency rules
$(LIB_CLIENT_OBJ): $(AUTODEP)
!if EXIST($(LIB_CLIENT_DST)\deps.mak)
!include $(LIB_CLIENT_DST)\deps.mak
!endif

# Build rules.  Use batch-mode rules if possible.
!if DEFINED(_NMAKE_VER)
{$(LIB_CLIENT_SRC)\}.cpp{$(LIB_CLIENT_DST)\}.obj::
!else
{$(LIB_CLIENT_SRC)\}.cpp{$(LIB_CLIENT_DST)\}.obj:
!endif
	@$(ECHO) Compile in $(LIB_CLIENT_SRC)
	-@$(MKDIR) $(LIB_CLIENT_DST) 2>NUL:
	$(cpp) $(cppdebug) $(cppflags) $(cppvarsmt) /showIncludes \
		$(LIB_CLIENT_INC) \
		/Fo$(LIB_CLIENT_DST)\ \
		/Fd$(LIB_CLIENT_LIB:.lib=.pdb) \
		$< | $(AUTODEP) $(LIB_CLIENT_SRC) $(LIB_CLIENT_DST)
$(LIB_CLIENT_LIB): $(LIB_CLIENT_OBJ)
	@$(ECHO) Link $(@F)
	$(implib) $(ildebug) $(ilflags) \
		/out:$@ \
		$**
	$(AUTODEP) $(LIB_CLIENT_SRC) $(LIB_CLIENT_DST) $(**:.obj=.d)

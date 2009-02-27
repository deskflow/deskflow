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

LIB_SERVER_SRC = lib\server
LIB_SERVER_DST = $(BUILD_DST)\$(LIB_SERVER_SRC)
LIB_SERVER_LIB = "$(LIB_SERVER_DST)\server.lib"
LIB_SERVER_CPP =					\
	"CBaseClientProxy.cpp"			\
	"CClientListener.cpp"			\
	"CClientProxy.cpp"				\
	"CClientProxy1_0.cpp"			\
	"CClientProxy1_1.cpp"			\
	"CClientProxy1_2.cpp"			\
	"CClientProxy1_3.cpp"			\
	"CClientProxyUnknown.cpp"		\
	"CConfig.cpp"					\
	"CInputFilter.cpp"				\
	"CPrimaryClient.cpp"			\
	"CServer.cpp"					\
	$(NULL)
LIB_SERVER_OBJ =									\
	"$(LIB_SERVER_DST)\CBaseClientProxy.obj"		\
	"$(LIB_SERVER_DST)\CClientListener.obj"			\
	"$(LIB_SERVER_DST)\CClientProxy.obj"			\
	"$(LIB_SERVER_DST)\CClientProxy1_0.obj"			\
	"$(LIB_SERVER_DST)\CClientProxy1_1.obj"			\
	"$(LIB_SERVER_DST)\CClientProxy1_2.obj"			\
	"$(LIB_SERVER_DST)\CClientProxy1_3.obj"			\
	"$(LIB_SERVER_DST)\CClientProxyUnknown.obj"		\
	"$(LIB_SERVER_DST)\CConfig.obj"					\
	"$(LIB_SERVER_DST)\CInputFilter.obj"			\
	"$(LIB_SERVER_DST)\CPrimaryClient.obj"			\
	"$(LIB_SERVER_DST)\CServer.obj"					\
	$(NULL)
LIB_SERVER_INC =					\
	/I"lib\common"					\
	/I"lib\arch"					\
	/I"lib\base"					\
	/I"lib\mt"						\
	/I"lib\io"						\
	/I"lib\net"						\
	/I"lib\synergy"					\
	/I"lib\platform"				\
	$(NULL)

CPP_FILES = $(CPP_FILES) $(LIB_SERVER_CPP)
OBJ_FILES = $(OBJ_FILES) $(LIB_SERVER_OBJ)
LIB_FILES = $(LIB_FILES) $(LIB_SERVER_LIB)

# Dependency rules
$(LIB_SERVER_OBJ): $(AUTODEP)
!if EXIST($(LIB_SERVER_DST)\deps.mak)
!include $(LIB_SERVER_DST)\deps.mak
!endif

# Build rules.  Use batch-mode rules if possible.
!if DEFINED(_NMAKE_VER)
{$(LIB_SERVER_SRC)\}.cpp{$(LIB_SERVER_DST)\}.obj::
!else
{$(LIB_SERVER_SRC)\}.cpp{$(LIB_SERVER_DST)\}.obj:
!endif
	@$(ECHO) Compile in $(LIB_SERVER_SRC)
	-@$(MKDIR) $(LIB_SERVER_DST) 2>NUL:
	$(cpp) $(cppdebug) $(cppflags) $(cppvarsmt) /showIncludes \
		$(LIB_SERVER_INC) \
		/Fo$(LIB_SERVER_DST)\ \
		/Fd$(LIB_SERVER_LIB:.lib=.pdb) \
		$< | $(AUTODEP) $(LIB_SERVER_SRC) $(LIB_SERVER_DST)
$(LIB_SERVER_LIB): $(LIB_SERVER_OBJ)
	@$(ECHO) Link $(@F)
	$(implib) $(ildebug) $(ilflags) \
		/out:$@ \
		$**
	$(AUTODEP) $(LIB_SERVER_SRC) $(LIB_SERVER_DST) $(**:.obj=.d)

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

LIB_NET_SRC = lib\net
LIB_NET_DST = $(BUILD_DST)\$(LIB_NET_SRC)
LIB_NET_LIB = "$(LIB_NET_DST)\net.lib"
LIB_NET_CPP =						\
	"CNetworkAddress.cpp"			\
	"CSocketMultiplexer.cpp"		\
	"CTCPListenSocket.cpp"			\
	"CTCPSocket.cpp"				\
	"CTCPSocketFactory.cpp"			\
	"IDataSocket.cpp"				\
	"IListenSocket.cpp"				\
	"ISocket.cpp"					\
	"XSocket.cpp"					\
	$(NULL)
LIB_NET_OBJ =									\
	"$(LIB_NET_DST)\CNetworkAddress.obj"		\
	"$(LIB_NET_DST)\CSocketMultiplexer.obj"		\
	"$(LIB_NET_DST)\CTCPListenSocket.obj"		\
	"$(LIB_NET_DST)\CTCPSocket.obj"				\
	"$(LIB_NET_DST)\CTCPSocketFactory.obj"		\
	"$(LIB_NET_DST)\IDataSocket.obj"			\
	"$(LIB_NET_DST)\IListenSocket.obj"			\
	"$(LIB_NET_DST)\ISocket.obj"				\
	"$(LIB_NET_DST)\XSocket.obj"				\
	$(NULL)
LIB_NET_INC =						\
	/I"lib\common"					\
	/I"lib\arch"					\
	/I"lib\base"					\
	/I"lib\mt"						\
	/I"lib\io"						\
	$(NULL)

CPP_FILES = $(CPP_FILES) $(LIB_NET_CPP)
OBJ_FILES = $(OBJ_FILES) $(LIB_NET_OBJ)
LIB_FILES = $(LIB_FILES) $(LIB_NET_LIB)

# Dependency rules
$(LIB_NET_OBJ): $(AUTODEP)
!if EXIST($(LIB_NET_DST)\deps.mak)
!include $(LIB_NET_DST)\deps.mak
!endif

# Build rules.  Use batch-mode rules if possible.
!if DEFINED(_NMAKE_VER)
{$(LIB_NET_SRC)\}.cpp{$(LIB_NET_DST)\}.obj::
!else
{$(LIB_NET_SRC)\}.cpp{$(LIB_NET_DST)\}.obj:
!endif
	@$(ECHO) Compile in $(LIB_NET_SRC)
	-@$(MKDIR) $(LIB_NET_DST) 2>NUL:
	$(cpp) $(cppdebug) $(cppflags) $(cppvarsmt) /showIncludes \
		$(LIB_NET_INC) \
		/Fo$(LIB_NET_DST)\ \
		/Fd$(LIB_NET_LIB:.lib=.pdb) \
		$< | $(AUTODEP) $(LIB_NET_SRC) $(LIB_NET_DST)
$(LIB_NET_LIB): $(LIB_NET_OBJ)
	@$(ECHO) Link $(@F)
	$(implib) $(ildebug) $(ilflags) \
		/out:$@ \
		$**
	$(AUTODEP) $(LIB_NET_SRC) $(LIB_NET_DST) $(**:.obj=.d)

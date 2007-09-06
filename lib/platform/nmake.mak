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

LIB_PLATFORM_SRC = lib\platform
LIB_PLATFORM_DST = $(BUILD_DST)\$(LIB_PLATFORM_SRC)
LIB_PLATFORM_LIB = "$(LIB_PLATFORM_DST)\platform.lib"
LIB_PLATFORM_CPP =								\
	"CMSWindowsClipboard.cpp"					\
	"CMSWindowsClipboardAnyTextConverter.cpp"	\
	"CMSWindowsClipboardBitmapConverter.cpp"	\
	"CMSWindowsClipboardHTMLConverter.cpp"		\
	"CMSWindowsClipboardTextConverter.cpp"		\
	"CMSWindowsClipboardUTF16Converter.cpp"		\
	"CMSWindowsDesks.cpp"						\
	"CMSWindowsEventQueueBuffer.cpp"			\
	"CMSWindowsKeyState.cpp"					\
	"CMSWindowsScreen.cpp"						\
	"CMSWindowsScreenSaver.cpp"					\
	"CMSWindowsUtil.cpp"						\
	$(NULL)
LIB_PLATFORM_OBJ =													\
	"$(LIB_PLATFORM_DST)\CMSWindowsClipboard.obj"					\
	"$(LIB_PLATFORM_DST)\CMSWindowsClipboardAnyTextConverter.obj"	\
	"$(LIB_PLATFORM_DST)\CMSWindowsClipboardBitmapConverter.obj"	\
	"$(LIB_PLATFORM_DST)\CMSWindowsClipboardHTMLConverter.obj"		\
	"$(LIB_PLATFORM_DST)\CMSWindowsClipboardTextConverter.obj"		\
	"$(LIB_PLATFORM_DST)\CMSWindowsClipboardUTF16Converter.obj"		\
	"$(LIB_PLATFORM_DST)\CMSWindowsDesks.obj"						\
	"$(LIB_PLATFORM_DST)\CMSWindowsEventQueueBuffer.obj"			\
	"$(LIB_PLATFORM_DST)\CMSWindowsKeyState.obj"					\
	"$(LIB_PLATFORM_DST)\CMSWindowsScreen.obj"						\
	"$(LIB_PLATFORM_DST)\CMSWindowsScreenSaver.obj"					\
	"$(LIB_PLATFORM_DST)\CMSWindowsUtil.obj"						\
	$(NULL)
LIB_PLATFORM_HOOK_CPP =						\
	"$(LIB_PLATFORM_SRC)\CSynergyHook.cpp"	\
	$(NULL)
LIB_PLATFORM_HOOK_OBJ =						\
	"$(LIB_PLATFORM_DST)\CSynergyHook.obj"	\
	$(NULL)
LIB_PLATFORM_HOOK_DLL = "$(BUILD_DST)\synrgyhk.dll"
LIB_PLATFORM_INC =					\
	/I"lib\common"					\
	/I"lib\arch"					\
	/I"lib\base"					\
	/I"lib\mt"						\
	/I"lib\io"						\
	/I"lib\net"						\
	/I"lib\synergy"					\
	$(NULL)

CPP_FILES = $(CPP_FILES) $(LIB_PLATFORM_CPP)
OBJ_FILES = $(OBJ_FILES) $(LIB_PLATFORM_OBJ)
LIB_FILES = $(LIB_FILES) $(LIB_PLATFORM_LIB) $(LIB_PLATFORM_HOOK_DLL)

# Hook should be as small as possible.
cpphookdebug = $(cppdebug:-Ox=-O1)

# Don't do security checks or run time error checking on hook.
cpphookflags = $(cppflags:-GS=)
cpphookdebug = $(cpphookdebug:/GZ=)
cpphookdebug = $(cpphookdebug:/RTC1=)

# Dependency rules
$(LIB_PLATFORM_OBJ): $(AUTODEP)
!if EXIST($(LIB_PLATFORM_DST)\deps.mak)
!include $(LIB_PLATFORM_DST)\deps.mak
!endif

# Build rules.  Use batch-mode rules if possible.
!if DEFINED(_NMAKE_VER)
{$(LIB_PLATFORM_SRC)\}.cpp{$(LIB_PLATFORM_DST)\}.obj::
!else
{$(LIB_PLATFORM_SRC)\}.cpp{$(LIB_PLATFORM_DST)\}.obj:
!endif
	@$(ECHO) Compile in $(LIB_PLATFORM_SRC)
	-@$(MKDIR) $(LIB_PLATFORM_DST) 2>NUL:
	$(cpp) $(cppdebug) $(cppflags) $(cppvarsmt) /showIncludes \
		$(LIB_PLATFORM_INC) \
		/Fo$(LIB_PLATFORM_DST)\ \
		/Fd$(LIB_PLATFORM_LIB:.lib=.pdb) \
		$< | $(AUTODEP) $(LIB_PLATFORM_SRC) $(LIB_PLATFORM_DST)
$(LIB_PLATFORM_LIB): $(LIB_PLATFORM_OBJ)
	@$(ECHO) Link $(@F)
	$(implib) $(ildebug) $(ilflags) \
		/out:$@ \
		$**
	$(AUTODEP) $(LIB_PLATFORM_SRC) $(LIB_PLATFORM_DST) \
		$(LIB_PLATFORM_OBJ:.obj=.d) $(LIB_PLATFORM_HOOK_OBJ:.obj=.d)

# Hook build rules
$(LIB_PLATFORM_HOOK_OBJ): \
		$(LIB_PLATFORM_HOOK_CPP) $(LIB_PLATFORM_HOOK_CPP:.cpp=.h)
	@$(ECHO) Compile $(LIB_PLATFORM_HOOK_CPP)
	-@$(MKDIR) $(LIB_PLATFORM_DST) 2>NUL:
	$(cpp) $(cpphookdebug) $(cpphookflags) $(cppvarsmt) /showIncludes \
		-D_DLL -D_USRDLL -DSYNRGYHK_EXPORTS \
		$(LIB_PLATFORM_INC) \
		/Fo$(LIB_PLATFORM_DST)\ \
		/Fd$(@:.obj=.pdb) \
		$(LIB_PLATFORM_HOOK_CPP) | \
			$(AUTODEP) $(LIB_PLATFORM_SRC) $(LIB_PLATFORM_DST)
$(LIB_PLATFORM_HOOK_DLL): $(LIB_PLATFORM_HOOK_OBJ)
	@$(ECHO) Link $(@F)
	$(link) $(ldebug) $(lflags) $(guilibsmt) \
		/entry:"DllMain$(DLLENTRY)" /dll \
		/out:$@ \
		$**
	$(AUTODEP) $(LIB_PLATFORM_SRC) $(LIB_PLATFORM_DST) \
		$(LIB_PLATFORM_OBJ:.obj=.d) $(LIB_PLATFORM_HOOK_OBJ:.obj=.d)

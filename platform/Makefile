DEPTH=..
include $(DEPTH)/Makecommon

#
# target file
#
TARGET = platform

#
# source files
#
LCXXINCS =				\
	-I$(DEPTH)/base 		\
	-I$(DEPTH)/mt 			\
	-I$(DEPTH)/synergy 		\
	$(NULL)
CXXFILES = 				\
	CPlatform.cpp			\
	CXWindowsClipboard.cpp		\
	CXWindowsScreen.cpp		\
	CXWindowsUtil.cpp		\
	$(NULL)

targets: $(LIBTARGET)

$(LIBTARGET): $(OBJECTS) $(DEPLIBS)
	if test ! -d $(LIBDIR); then $(MKDIR) $(LIBDIR); fi
	$(ARF) $(LIBTARGET) $(OBJECTS)

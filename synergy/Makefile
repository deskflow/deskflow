DEPTH=..
include $(DEPTH)/Makecommon

#
# target file
#
TARGET = synergy

#
# source files
#
LCXXINCS =				\
	-I$(DEPTH)/base 		\
	-I$(DEPTH)/mt	 		\
	-I$(DEPTH)/io	 		\
	-I$(DEPTH)/net	 		\
	$(NULL)
CXXFILES = 				\
	CInputPacketStream.cpp		\
	COutputPacketStream.cpp		\
	CProtocolUtil.cpp		\
	CClipboard.cpp			\
	CTCPSocketFactory.cpp		\
	CXWindowsClipboard.cpp		\
	CXWindowsScreen.cpp		\
	XSynergy.cpp			\
	$(NULL)

targets: $(LIBTARGET)

$(LIBTARGET): $(OBJECTS)
	if test ! -d $(LIBDIR); then $(MKDIR) $(LIBDIR); fi
	$(ARF) $(LIBTARGET) $(OBJECTS)


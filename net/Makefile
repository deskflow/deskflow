DEPTH=..
include $(DEPTH)/Makecommon

#
# target file
#
TARGET = net

#
# source files
#
LCXXINCS =				\
	-I$(DEPTH)/base 		\
	-I$(DEPTH)/mt	 		\
	-I$(DEPTH)/io	 		\
	$(NULL)
CXXFILES = 				\
	XSocket.cpp			\
	CNetworkAddress.cpp		\
	CTCPSocket.cpp			\
	CTCPListenSocket.cpp		\
	$(NULL)

targets: $(LIBTARGET)

$(LIBTARGET): $(OBJECTS)
	if test ! -d $(LIBDIR); then $(MKDIR) $(LIBDIR); fi
	$(ARF) $(LIBTARGET) $(OBJECTS)

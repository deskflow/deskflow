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
	CNetwork.cpp			\
	CNetworkAddress.cpp		\
	CTCPSocket.cpp			\
	CTCPListenSocket.cpp		\
	XSocket.cpp			\
	$(NULL)

targets: $(LIBTARGET)

$(LIBTARGET): $(OBJECTS)
	if test ! -d $(LIBDIR); then $(MKDIR) $(LIBDIR); fi
	$(ARF) $(LIBTARGET) $(OBJECTS)

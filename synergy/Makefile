DEPTH=..
include $(DEPTH)/Makecommon

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
	CTCPSocketFactory.cpp		\
	CProtocolUtil.cpp		\
	CClient.cpp			\
	CServerProtocol.cpp		\
	CServerProtocol1_0.cpp		\
	CScreenMap.cpp			\
	CServer.cpp			\
	CXWindowsScreen.cpp		\
	CXWindowsPrimaryScreen.cpp	\
	CXWindowsSecondaryScreen.cpp	\
	XSynergy.cpp			\
	$(NULL)

#
# libraries we depend on
#
DEPLIBS =				\
	$(LIBDIR)/libnet.a		\
	$(LIBDIR)/libio.a		\
	$(LIBDIR)/libmt.a		\
	$(LIBDIR)/libbase.a		\
	$(NULL)
LLDLIBS  =				\
	$(DEPLIBS)			\
	-lpthread			\
	$(NULL)

targets: server client

server: server.o $(OBJECTS) $(DEPLIBS)
	$(CXX) $(CXXFLAGS) -o $@ server.o $(OBJECTS) $(LDFLAGS)

client: client.o $(OBJECTS) $(DEPLIBS)
	$(CXX) $(CXXFLAGS) -o $@ client.o $(OBJECTS) $(LDFLAGS)


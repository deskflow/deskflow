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
	CServerProtocol.cpp		\
	CServerProtocol1_0.cpp		\
	CScreenMap.cpp			\
	CServer.cpp			\
	CXWindowsPrimaryScreen.cpp	\
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

targets: server

server: $(OBJECTS) $(DEPLIBS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJECTS) $(LDFLAGS)


DEPTH=..
include $(DEPTH)/Makecommon

#
# target file
#
TARGET = server

#
# source files
#
LCXXINCS =				\
	-I$(DEPTH)/base 		\
	-I$(DEPTH)/mt	 		\
	-I$(DEPTH)/io	 		\
	-I$(DEPTH)/http 		\
	-I$(DEPTH)/net	 		\
	-I$(DEPTH)/synergy		\
	$(NULL)
CXXFILES = 				\
	CScreenMap.cpp			\
	CServerProtocol.cpp		\
	CServerProtocol1_0.cpp		\
	CXWindowsPrimaryScreen.cpp	\
	CServer.cpp			\
	CHTTPServer.cpp			\
	server.cpp			\
	$(NULL)

#
# libraries we depend on
#
DEPLIBS =				\
	$(LIBDIR)/libsynergy.a		\
	$(LIBDIR)/libnet.a		\
	$(LIBDIR)/libhttp.a		\
	$(LIBDIR)/libio.a		\
	$(LIBDIR)/libmt.a		\
	$(LIBDIR)/libbase.a		\
	$(NULL)
LLDLIBS  =				\
	$(DEPLIBS)			\
	-lpthread			\
	$(NULL)

targets: $(BINTARGET)

$(BINTARGET): $(OBJECTS) $(DEPLIBS)
	if test ! -d $(BINDIR); then $(MKDIR) $(BINDIR); fi
	$(CXX) $(CXXFLAGS) -o $@ $(OBJECTS) $(LDFLAGS)


DEPTH=.
include Make-linux

#
# target files
#
TARGETS  = main

#
# source files
#
CXXFILES = 				\
	XBase.cpp			\
	CTrace.cpp			\
	CEventQueue.cpp			\
	CSocket.cpp			\
	CMessageSocket.cpp		\
	CSocketFactory.cpp		\
	CServer.cpp			\
	CClient.cpp			\
	CScreenProxy.cpp		\
	CXScreen.cpp			\
	CUnixXScreen.cpp		\
	CUnixTCPSocket.cpp		\
	CUnixEventQueue.cpp		\
	main.cpp			\
	$(NULL)

#
# libraries we depend on
#
DEPLIBS  = \
	$(NULL)

targets: $(TARGETS)

main: $(OBJECTS) $(DEPLIBS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJECTS) $(LDFLAGS)

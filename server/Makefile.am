## Process this file with automake to produce Makefile.in
NULL =
DEPTH = ..

bin_PROGRAMS = synergyd
synergyd_SOURCES = 					\
	CConfig.cpp						\
	CServerProtocol.cpp				\
	CServerProtocol1_0.cpp			\
	CXWindowsPrimaryScreen.cpp		\
	CServer.cpp						\
	CHTTPServer.cpp					\
	server.cpp						\
	CConfig.h						\
	CHTTPServer.h					\
	CServer.h						\
	CServerProtocol.h				\
	CServerProtocol1_0.h			\
	CSynergyHook.h					\
	CXWindowsPrimaryScreen.h		\
	$(NULL)
synergyd_LDADD =					\
	$(DEPTH)/platform/libplatform.a	\
	$(DEPTH)/synergy/libsynergy.a	\
	$(DEPTH)/net/libnet.a			\
	$(DEPTH)/http/libhttp.a			\
	$(DEPTH)/io/libio.a				\
	$(DEPTH)/mt/libmt.a				\
	$(DEPTH)/base/libbase.a			\
	$(X_LIBS)						\
	$(X_PRE_LIBS)					\
	-lX11							\
	$(X_EXTRA_LIBS)					\
	$(NULL)
INCLUDES =					\
	-I$(DEPTH)/base 		\
	-I$(DEPTH)/mt	 		\
	-I$(DEPTH)/io	 		\
	-I$(DEPTH)/http 		\
	-I$(DEPTH)/net	 		\
	-I$(DEPTH)/synergy		\
	-I$(DEPTH)/platform 	\
	$(NULL)

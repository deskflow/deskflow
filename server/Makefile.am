## Process this file with automake to produce Makefile.in
NULL =
DEPTH = ..

bin_PROGRAMS = synergyd
synergyd_SOURCES = 					\
	CClientProxy.cpp				\
	CClientProxy1_0.cpp				\
	CConfig.cpp						\
	CHTTPServer.cpp					\
	CPrimaryClient.cpp				\
	CServer.cpp						\
	CXWindowsPrimaryScreen.cpp		\
	server.cpp						\
	CClientProxy.h					\
	CClientProxy1_0.h				\
	CConfig.h						\
	CHTTPServer.h					\
	CPrimaryClient.h				\
	CServer.h						\
	CXWindowsPrimaryScreen.h		\
	IPrimaryReceiver.h				\
	IPrimaryScreen.h				\
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
	-lXtst							\
	-lXext							\
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

## Process this file with automake to produce Makefile.in
NULL =
DEPTH = ../..
VDEPTH = ./$(VPATH)/$(DEPTH)

EXTRA_DIST =						\
	server.dsp						\
	CMSWindowsPrimaryScreen.cpp		\
	CMSWindowsPrimaryScreen.h		\
	$(NULL)

MAINTAINERCLEANFILES =				\
	Makefile.in						\
	$(NULL)

noinst_LIBRARIES = libserver.a
libserver_a_SOURCES = 				\
	CClientProxy.cpp				\
	CClientProxy1_0.cpp				\
	CConfig.cpp						\
	CHTTPServer.cpp					\
	CPrimaryClient.cpp				\
	CPrimaryScreen.cpp				\
	CServer.cpp						\
	CXWindowsPrimaryScreen.cpp		\
	CClientProxy.h					\
	CClientProxy1_0.h				\
	CConfig.h						\
	CHTTPServer.h					\
	CPrimaryClient.h				\
	CPrimaryScreen.h				\
	CServer.h						\
	CXWindowsPrimaryScreen.h		\
	IPrimaryScreenFactory.h			\
	$(NULL)
INCLUDES =							\
	-I$(VDEPTH)/lib/base 			\
	-I$(VDEPTH)/lib/mt	 			\
	-I$(VDEPTH)/lib/io	 			\
	-I$(VDEPTH)/lib/http 			\
	-I$(VDEPTH)/lib/net	 			\
	-I$(VDEPTH)/lib/synergy			\
	-I$(VDEPTH)/lib/platform 		\
	$(NULL)

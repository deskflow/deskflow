## Process this file with automake to produce Makefile.in
NULL =
DEPTH = ../..

EXTRA_DIST =						\
	client.dsp						\
	CMSWindowsSecondaryScreen.cpp	\
	CMSWindowsSecondaryScreen.h		\
	$(NULL)

MAINTAINERCLEANFILES =				\
	Makefile.in						\
	$(NULL)

noinst_LIBRARIES = libclient.a
libclient_a_SOURCES = 				\
	CClient.cpp						\
	CSecondaryScreen.cpp			\
	CServerProxy.cpp				\
	CXWindowsSecondaryScreen.cpp	\
	CClient.h						\
	CSecondaryScreen.h				\
	CServerProxy.h					\
	CXWindowsSecondaryScreen.h		\
	ISecondaryScreenFactory.h		\
	$(NULL)
INCLUDES =							\
	-I$(DEPTH)/lib/base 			\
	-I$(DEPTH)/lib/mt	 			\
	-I$(DEPTH)/lib/io	 			\
	-I$(DEPTH)/lib/net	 			\
	-I$(DEPTH)/lib/synergy			\
	-I$(DEPTH)/lib/platform 		\
	$(NULL)

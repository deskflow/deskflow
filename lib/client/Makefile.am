## Process this file with automake to produce Makefile.in
NULL =
DEPTH = ../..
VDEPTH = ./$(VPATH)/$(DEPTH)

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
	-I$(VDEPTH)/lib/base 			\
	-I$(VDEPTH)/lib/mt	 			\
	-I$(VDEPTH)/lib/io	 			\
	-I$(VDEPTH)/lib/net	 			\
	-I$(VDEPTH)/lib/synergy			\
	-I$(VDEPTH)/lib/platform 		\
	$(NULL)

## Process this file with automake to produce Makefile.in
NULL =
DEPTH = ../..
VDEPTH = ./$(VPATH)/$(DEPTH)

EXTRA_DIST =				\
	http.dsp				\
	$(NULL)

MAINTAINERCLEANFILES =		\
	Makefile.in				\
	$(NULL)

noinst_LIBRARIES = libhttp.a
libhttp_a_SOURCES = 		\
	CHTTPProtocol.cpp		\
	XHTTP.cpp				\
	CHTTPProtocol.h			\
	XHTTP.h					\
	$(NULL)
INCLUDES =					\
	-I$(VDEPTH)/lib/base 	\
	-I$(VDEPTH)/lib/mt	 	\
	-I$(VDEPTH)/lib/io	 	\
	$(NULL)

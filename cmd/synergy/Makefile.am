## Process this file with automake to produce Makefile.in
NULL =
DEPTH = ../..

EXTRA_DIST =				\
	synergy.dsp				\
	synergy.rc				\
	resource.h				\
	$(NULL)

MAINTAINERCLEANFILES =		\
	Makefile.in				\
	$(NULL)

bin_PROGRAMS = synergy
synergy_SOURCES = 						\
	synergy.cpp							\
	$(NULL)
synergy_LDADD =							\
	$(DEPTH)/lib/client/libclient.a		\
	$(DEPTH)/lib/platform/libplatform.a	\
	$(DEPTH)/lib/synergy/libsynergy.a	\
	$(DEPTH)/lib/net/libnet.a			\
	$(DEPTH)/lib/io/libio.a				\
	$(DEPTH)/lib/mt/libmt.a				\
	$(DEPTH)/lib/base/libbase.a			\
	$(X_LIBS)							\
	$(X_PRE_LIBS)						\
	-lXtst								\
	-lXext								\
	-lX11								\
	$(X_EXTRA_LIBS)						\
	$(NULL)
INCLUDES =								\
	-I$(DEPTH)/lib/base 				\
	-I$(DEPTH)/lib/mt	 				\
	-I$(DEPTH)/lib/io	 				\
	-I$(DEPTH)/lib/net	 				\
	-I$(DEPTH)/lib/synergy				\
	-I$(DEPTH)/lib/platform 			\
	-I$(DEPTH)/lib/client 				\
	$(NULL)

## Process this file with automake to produce Makefile.in
NULL =
DEPTH = ../..
VDEPTH = ./$(VPATH)/$(DEPTH)

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
	-I$(VDEPTH)/lib/base 				\
	-I$(VDEPTH)/lib/mt	 				\
	-I$(VDEPTH)/lib/io	 				\
	-I$(VDEPTH)/lib/net	 				\
	-I$(VDEPTH)/lib/synergy				\
	-I$(VDEPTH)/lib/platform 			\
	-I$(VDEPTH)/lib/client 				\
	$(NULL)

## Process this file with automake to produce Makefile.in
NULL =
DEPTH = ..

bin_PROGRAMS = synergy
synergy_SOURCES = 			\
	CXWindowsSecondaryScreen.cpp	\
	CClient.cpp				\
	client.cpp				\
	$(NULL)
synergy_LDADD =						\
	$(DEPTH)/platform/libplatform.a	\
	$(DEPTH)/synergy/libsynergy.a	\
	$(DEPTH)/net/libnet.a			\
	$(DEPTH)/io/libio.a				\
	$(DEPTH)/mt/libmt.a				\
	$(DEPTH)/base/libbase.a			\
	-lX11							\
	-lXext							\
	-lXtst							\
	$(NULL)
INCLUDES =					\
	-I$(DEPTH)/base 		\
	-I$(DEPTH)/mt	 		\
	-I$(DEPTH)/io	 		\
	-I$(DEPTH)/net	 		\
	-I$(DEPTH)/synergy		\
	-I$(DEPTH)/platform 	\
	$(NULL)

## Process this file with automake to produce Makefile.in
NULL =
DEPTH = ..

bin_PROGRAMS = synergy
synergy_SOURCES = 					\
	CClient.cpp						\
	CSecondaryScreen.cpp			\
	CServerProxy.cpp				\
	CXWindowsSecondaryScreen.cpp	\
	client.cpp						\
	CClient.h						\
	CSecondaryScreen.h				\
	CServerProxy.h					\
	CXWindowsSecondaryScreen.h		\
	$(NULL)
synergy_LDADD =						\
	$(DEPTH)/platform/libplatform.a	\
	$(DEPTH)/synergy/libsynergy.a	\
	$(DEPTH)/net/libnet.a			\
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
	-I$(DEPTH)/net	 		\
	-I$(DEPTH)/synergy		\
	-I$(DEPTH)/platform 	\
	$(NULL)

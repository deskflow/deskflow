## Process this file with automake to produce Makefile.in
NULL =
DEPTH = ../..

EXTRA_DIST =				\
	net.dsp					\
	$(NULL)

MAINTAINERCLEANFILES =		\
	Makefile.in				\
	$(NULL)

noinst_LIBRARIES = libnet.a
libnet_a_SOURCES = 			\
	CNetwork.cpp			\
	CNetworkAddress.cpp		\
	CTCPListenSocket.cpp	\
	CTCPSocket.cpp			\
	CTCPSocketFactory.cpp	\
	XNetwork.cpp			\
	XSocket.cpp				\
	CNetwork.h				\
	CNetworkAddress.h		\
	CTCPListenSocket.h		\
	CTCPSocket.h			\
	CTCPSocketFactory.h		\
	IDataSocket.h			\
	IListenSocket.h			\
	ISocket.h				\
	ISocketFactory.h		\
	XNetwork.h				\
	XSocket.h				\
	$(NULL)
INCLUDES =					\
	-I$(DEPTH)/lib/base 	\
	-I$(DEPTH)/lib/mt	 	\
	-I$(DEPTH)/lib/io	 	\
	$(NULL)

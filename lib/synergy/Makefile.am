## Process this file with automake to produce Makefile.in
NULL =
DEPTH = ../..

EXTRA_DIST =					\
	libsynergy.dsp				\
	$(NULL)

noinst_LIBRARIES = libsynergy.a
libsynergy_a_SOURCES = 			\
	CInputPacketStream.cpp		\
	COutputPacketStream.cpp		\
	CProtocolUtil.cpp			\
	CClipboard.cpp				\
	XScreen.cpp					\
	XSynergy.cpp				\
	CClipboard.h				\
	CInputPacketStream.h		\
	COutputPacketStream.h		\
	CProtocolUtil.h				\
	ClipboardTypes.h			\
	IClient.h					\
	IClipboard.h				\
	IPrimaryScreenReceiver.h	\
	IScreen.h					\
	IScreenEventHandler.h		\
	IScreenReceiver.h			\
	IScreenSaver.h				\
	IServer.h					\
	KeyTypes.h					\
	MouseTypes.h				\
	ProtocolTypes.h				\
	Version.h					\
	XScreen.h					\
	XSynergy.h					\
	$(NULL)
INCLUDES =						\
	-I$(DEPTH)/lib/base 		\
	-I$(DEPTH)/lib/mt	 		\
	-I$(DEPTH)/lib/io	 		\
	-I$(DEPTH)/lib/net	 		\
	$(NULL)

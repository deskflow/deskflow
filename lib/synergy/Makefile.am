## Process this file with automake to produce Makefile.in
NULL =
DEPTH = ../..
VDEPTH = ./$(VPATH)/$(DEPTH)

EXTRA_DIST =					\
	libsynergy.dsp				\
	$(NULL)

MAINTAINERCLEANFILES =			\
	Makefile.in					\
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
	XScreen.h					\
	XSynergy.h					\
	$(NULL)
INCLUDES =						\
	-I$(VDEPTH)/lib/base 		\
	-I$(VDEPTH)/lib/mt	 		\
	-I$(VDEPTH)/lib/io	 		\
	-I$(VDEPTH)/lib/net	 		\
	$(NULL)

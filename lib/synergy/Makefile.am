# synergy -- mouse and keyboard sharing utility
# Copyright (C) 2002 Chris Schoeneman
# 
# This package is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# found in the file COPYING that should have accompanied this file.
# 
# This package is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

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
	CClipboard.cpp				\
	CPacketStreamFilter.cpp		\
	CProtocolUtil.cpp			\
	CScreen.cpp					\
	IClipboard.cpp				\
	IPrimaryScreen.cpp			\
	IScreen.cpp					\
	XScreen.cpp					\
	XSynergy.cpp				\
	CClipboard.h				\
	CPacketStreamFilter.h		\
	CProtocolUtil.h				\
	CScreen.h					\
	ClipboardTypes.h			\
	IClient.h					\
	IClipboard.h				\
	IKeyState.h					\
	IPlatformScreen.h			\
	IPrimaryScreen.h			\
	IScreen.h					\
	IScreenSaver.h				\
	ISecondaryScreen.h			\
	KeyTypes.h					\
	MouseTypes.h				\
	OptionTypes.h				\
	ProtocolTypes.h				\
	XScreen.h					\
	XSynergy.h					\
	$(NULL)
INCLUDES =						\
	-I$(VDEPTH)/lib/common		\
	-I$(VDEPTH)/lib/arch		\
	-I$(VDEPTH)/lib/base 		\
	-I$(VDEPTH)/lib/mt	 		\
	-I$(VDEPTH)/lib/io	 		\
	-I$(VDEPTH)/lib/net	 		\
	$(NULL)

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

EXTRA_DIST =					\
	libsynergy.dsp				\
	$(NULL)

MAINTAINERCLEANFILES =			\
	Makefile.in					\
	$(NULL)

noinst_LIBRARIES = libsynergy.a
libsynergy_a_SOURCES = 			\
	CClipboard.cpp				\
	CKeyState.cpp				\
	CPacketStreamFilter.cpp		\
	CPlatformScreen.cpp			\
	CProtocolUtil.cpp			\
	CScreen.cpp					\
	IClipboard.cpp				\
	IKeyState.cpp				\
	IPrimaryScreen.cpp			\
	IScreen.cpp					\
	ProtocolTypes.cpp			\
	XScreen.cpp					\
	XSynergy.cpp				\
	CClipboard.h				\
	CKeyState.h					\
	CPacketStreamFilter.h		\
	CPlatformScreen.h			\
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
	-I$(top_srcdir)/lib/common	\
	-I$(top_srcdir)/lib/arch	\
	-I$(top_srcdir)/lib/base 	\
	-I$(top_srcdir)/lib/mt	 	\
	-I$(top_srcdir)/lib/io	 	\
	-I$(top_srcdir)/lib/net	 	\
	$(NULL)

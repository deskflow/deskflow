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

EXTRA_DIST =								\
	makehook.dsp							\
	platform.dsp							\
	synrgyhk.dsp							\
	CMSWindowsClipboard.cpp					\
	CMSWindowsClipboardAnyTextConverter.cpp	\
	CMSWindowsClipboardTextConverter.cpp	\
	CMSWindowsClipboardUTF16Converter.cpp	\
	CMSWindowsPrimaryScreen.cpp				\
	CMSWindowsScreen.cpp					\
	CMSWindowsScreenSaver.cpp				\
	CMSWindowsSecondaryScreen.cpp			\
	CSynergyHook.cpp						\
	CMSWindowsClipboard.h					\
	CMSWindowsClipboardAnyTextConverter.h	\
	CMSWindowsClipboardTextConverter.h		\
	CMSWindowsClipboardUTF16Converter.h		\
	CMSWindowsPrimaryScreen.h				\
	CMSWindowsScreen.h						\
	CMSWindowsScreenSaver.h					\
	CMSWindowsSecondaryScreen.h				\
	CSynergyHook.h							\
	IMSWindowsScreenEventHandler.h			\
	$(NULL)

MAINTAINERCLEANFILES =						\
	Makefile.in								\
	$(NULL)

noinst_LIBRARIES = libplatform.a
libplatform_a_SOURCES = 					\
	CXWindowsClipboard.cpp					\
	CXWindowsClipboardTextConverter.cpp		\
	CXWindowsClipboardUCS2Converter.cpp		\
	CXWindowsClipboardUTF8Converter.cpp		\
	CXWindowsPrimaryScreen.cpp				\
	CXWindowsScreen.cpp						\
	CXWindowsScreenSaver.cpp				\
	CXWindowsSecondaryScreen.cpp			\
	CXWindowsUtil.cpp						\
	CXWindowsClipboard.h					\
	CXWindowsClipboardTextConverter.h		\
	CXWindowsClipboardUCS2Converter.h		\
	CXWindowsClipboardUTF8Converter.h		\
	CXWindowsPrimaryScreen.h				\
	CXWindowsScreen.h						\
	CXWindowsScreenSaver.h					\
	CXWindowsSecondaryScreen.h				\
	CXWindowsUtil.h							\
	$(NULL)
INCLUDES =									\
	-I$(VDEPTH)/lib/common					\
	-I$(VDEPTH)/lib/arch					\
	-I$(VDEPTH)/lib/base 					\
	-I$(VDEPTH)/lib/mt	 					\
	-I$(VDEPTH)/lib/io	 					\
	-I$(VDEPTH)/lib/synergy 				\
	$(NULL)

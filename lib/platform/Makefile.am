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

XWINDOWS_SOURCE_FILES =						\
	CXWindowsClipboard.cpp					\
	CXWindowsClipboardAnyBitmapConverter.cpp\
	CXWindowsClipboardBMPConverter.cpp		\
	CXWindowsClipboardHTMLConverter.cpp		\
	CXWindowsClipboardTextConverter.cpp		\
	CXWindowsClipboardUCS2Converter.cpp		\
	CXWindowsClipboardUTF8Converter.cpp		\
	CXWindowsEventQueueBuffer.cpp			\
	CXWindowsKeyState.cpp					\
	CXWindowsScreen.cpp						\
	CXWindowsScreenSaver.cpp				\
	CXWindowsUtil.cpp						\
	CXWindowsClipboard.h					\
	CXWindowsClipboardAnyBitmapConverter.h	\
	CXWindowsClipboardBMPConverter.h		\
	CXWindowsClipboardHTMLConverter.h		\
	CXWindowsClipboardTextConverter.h		\
	CXWindowsClipboardUCS2Converter.h		\
	CXWindowsClipboardUTF8Converter.h		\
	CXWindowsEventQueueBuffer.h				\
	CXWindowsKeyState.h						\
	CXWindowsScreen.h						\
	CXWindowsScreenSaver.h					\
	CXWindowsUtil.h							\
	$(NULL)
MSWINDOWS_SOURCE_FILES =					\
	CMSWindowsClipboard.cpp					\
	CMSWindowsClipboardAnyTextConverter.cpp	\
	CMSWindowsClipboardBitmapConverter.cpp	\
	CMSWindowsClipboardHTMLConverter.cpp	\
	CMSWindowsClipboardTextConverter.cpp	\
	CMSWindowsClipboardUTF16Converter.cpp	\
	CMSWindowsDesks.cpp						\
	CMSWindowsEventQueueBuffer.cpp			\
	CMSWindowsKeyState.cpp					\
	CMSWindowsScreen.cpp					\
	CMSWindowsScreenSaver.cpp				\
	CMSWindowsUtil.cpp						\
	CMSWindowsClipboard.h					\
	CMSWindowsClipboardAnyTextConverter.h	\
	CMSWindowsClipboardBitmapConverter.h	\
	CMSWindowsClipboardHTMLConverter.h		\
	CMSWindowsClipboardTextConverter.h		\
	CMSWindowsClipboardUTF16Converter.h		\
	CMSWindowsDesks.h						\
	CMSWindowsEventQueueBuffer.h			\
	CMSWindowsKeyState.h					\
	CMSWindowsScreen.h						\
	CMSWindowsScreenSaver.h					\
	CMSWindowsUtil.h						\
	$(NULL)
MSWINDOWS_HOOK_SOURCE_FILES =				\
	CSynergyHook.cpp						\
	CSynergyHook.h							\
	$(NULL)
CARBON_SOURCE_FILES =						\
	COSXClipboard.cpp						\
	COSXClipboardAnyTextConverter.cpp		\
	COSXClipboardTextConverter.cpp			\
	COSXClipboardUTF16Converter.cpp			\
	COSXEventQueueBuffer.cpp				\
	COSXKeyState.cpp						\
	COSXScreen.cpp							\
	COSXScreenSaver.cpp						\
	COSXScreenSaverUtil.m					\
	COSXClipboard.h							\
	COSXClipboardAnyTextConverter.h			\
	COSXClipboardTextConverter.h			\
	COSXClipboardUTF16Converter.h			\
	COSXEventQueueBuffer.h					\
	COSXKeyState.h							\
	COSXScreen.h							\
	COSXScreenSaver.h						\
	COSXScreenSaverUtil.h					\
	OSXScreenSaverControl.h					\
	$(NULL)

EXTRA_DIST =								\
	makehook.dsp							\
	platform.dsp							\
	synrgyhk.dsp							\
	$(XWINDOWS_SOURCE_FILES)				\
	$(MSWINDOWS_SOURCE_FILES)				\
	$(MSWINDOWS_HOOK_SOURCE_FILES)			\
	$(CARBON_SOURCE_FILES)					\
	$(NULL)

MAINTAINERCLEANFILES =						\
	Makefile.in								\
	$(NULL)

noinst_LIBRARIES = libplatform.a
if XWINDOWS
libplatform_a_SOURCES = $(XWINDOWS_SOURCE_FILES)
endif
if MSWINDOWS
libplatform_a_SOURCES = $(MSWINDOWS_SOURCE_FILES)
endif
if CARBON
libplatform_a_SOURCES = $(CARBON_SOURCE_FILES)
endif
INCLUDES =									\
	-I$(top_srcdir)/lib/common				\
	-I$(top_srcdir)/lib/arch				\
	-I$(top_srcdir)/lib/base 				\
	-I$(top_srcdir)/lib/mt	 				\
	-I$(top_srcdir)/lib/io	 				\
	-I$(top_srcdir)/lib/synergy 			\
	$(NULL)

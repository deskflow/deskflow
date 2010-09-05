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

MSWINDOWS_SOURCE_FILES =	\
	CAddScreen.cpp			\
	CAdvancedOptions.cpp	\
	CAutoStart.cpp			\
	CGlobalOptions.cpp		\
	CInfo.cpp				\
	CScreensLinks.cpp		\
	LaunchUtil.cpp			\
	launcher.cpp			\
	CAddScreen.h			\
	CAdvancedOptions.h		\
	CAutoStart.h			\
	CGlobalOptions.h		\
	CInfo.h					\
	CScreensLinks.h			\
	LaunchUtil.h			\
	resource.h				\
	launcher.rc				\
	$(NULL)

EXTRA_DIST =					\
	launcher.dsp				\
	synergy.ico					\
	$(MSWINDOWS_SOURCE_FILES)	\
	$(NULL)

MAINTAINERCLEANFILES =			\
	Makefile.in					\
	$(NULL)

if MSWINDOWS
bin_PROGRAMS = synergy
synergy_SOURCES =				\
	$(MSWINDOWS_SOURCE_FILES)	\
	$(NULL)
endif
synergy_LDADD =									\
	$(top_builddir)/lib/server/libserver.a		\
	$(top_builddir)/lib/platform/libplatform.a	\
	$(top_builddir)/lib/synergy/libsynergy.a	\
	$(top_builddir)/lib/net/libnet.a			\
	$(top_builddir)/lib/io/libio.a				\
	$(top_builddir)/lib/mt/libmt.a				\
	$(top_builddir)/lib/base/libbase.a			\
	$(top_builddir)/lib/common/libcommon.a		\
	$(top_builddir)/lib/arch/libarch.a			\
	$(NULL)
INCLUDES =								\
	-I$(top_srcdir)/lib/common			\
	-I$(top_srcdir)/lib/arch			\
	-I$(top_srcdir)/lib/base 			\
	-I$(top_srcdir)/lib/mt	 			\
	-I$(top_srcdir)/lib/io	 			\
	-I$(top_srcdir)/lib/net	 			\
	-I$(top_srcdir)/lib/synergy			\
	-I$(top_srcdir)/lib/platform 		\
	-I$(top_srcdir)/lib/server 			\
	$(NULL)

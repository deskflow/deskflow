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

EXTRA_DIST =							\
	CMSWindowsServerTaskBarReceiver.cpp	\
	CMSWindowsServerTaskBarReceiver.h	\
	resource.h							\
	synergys.ico						\
	synergys.dsp						\
	synergys.rc							\
	tb_error.ico						\
	tb_idle.ico							\
	tb_run.ico							\
	tb_wait.ico							\
	$(NULL)

MAINTAINERCLEANFILES =					\
	Makefile.in							\
	$(NULL)

bin_PROGRAMS = synergys
synergys_SOURCES = 						\
	CServerTaskBarReceiver.cpp			\
	CServerTaskBarReceiver.h			\
	CXWindowsServerTaskBarReceiver.cpp	\
	CXWindowsServerTaskBarReceiver.h	\
	synergys.cpp						\
	$(NULL)
synergys_LDADD =								\
	$(top_builddir)/lib/server/libserver.a		\
	$(top_builddir)/lib/platform/libplatform.a	\
	$(top_builddir)/lib/synergy/libsynergy.a	\
	$(top_builddir)/lib/net/libnet.a			\
	$(top_builddir)/lib/io/libio.a				\
	$(top_builddir)/lib/mt/libmt.a				\
	$(top_builddir)/lib/base/libbase.a			\
	$(top_builddir)/lib/common/libcommon.a		\
	$(top_builddir)/lib/arch/libarch.a			\
	$(X_LIBS)									\
	$(X_PRE_LIBS)								\
	-lXtst										\
	-lXext										\
	-lX11										\
	$(X_EXTRA_LIBS)								\
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
	-I$(top_srcdir)/lib/server	 		\
	$(NULL)

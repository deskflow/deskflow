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
synergys_LDADD =						\
	$(DEPTH)/lib/server/libserver.a		\
	$(DEPTH)/lib/platform/libplatform.a	\
	$(DEPTH)/lib/synergy/libsynergy.a	\
	$(DEPTH)/lib/net/libnet.a			\
	$(DEPTH)/lib/http/libhttp.a			\
	$(DEPTH)/lib/io/libio.a				\
	$(DEPTH)/lib/mt/libmt.a				\
	$(DEPTH)/lib/base/libbase.a			\
	$(DEPTH)/lib/arch/libarch.a			\
	$(X_LIBS)							\
	$(X_PRE_LIBS)						\
	-lXtst								\
	-lXext								\
	-lX11								\
	$(X_EXTRA_LIBS)						\
	$(NULL)
INCLUDES =								\
	-I$(VDEPTH)/lib/common				\
	-I$(VDEPTH)/lib/arch				\
	-I$(VDEPTH)/lib/base 				\
	-I$(VDEPTH)/lib/mt	 				\
	-I$(VDEPTH)/lib/io	 				\
	-I$(VDEPTH)/lib/http 				\
	-I$(VDEPTH)/lib/net	 				\
	-I$(VDEPTH)/lib/synergy				\
	-I$(VDEPTH)/lib/platform 			\
	-I$(VDEPTH)/lib/server	 			\
	$(NULL)

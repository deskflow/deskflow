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

EXTRA_DIST =						\
	server.dsp						\
	$(NULL)

MAINTAINERCLEANFILES =				\
	Makefile.in						\
	$(NULL)

noinst_LIBRARIES = libserver.a
libserver_a_SOURCES = 				\
	CClientListener.cpp				\
	CClientProxy.cpp				\
	CClientProxy1_0.cpp				\
	CClientProxy1_1.cpp				\
	CClientProxyUnknown.cpp			\
	CConfig.cpp						\
	CPrimaryClient.cpp				\
	CServer.cpp						\
	CClientListener.h				\
	CClientProxy.h					\
	CClientProxy1_0.h				\
	CClientProxy1_1.h				\
	CClientProxyUnknown.h			\
	CConfig.h						\
	CPrimaryClient.h				\
	CServer.h						\
	$(NULL)
INCLUDES =							\
	-I$(VDEPTH)/lib/common			\
	-I$(VDEPTH)/lib/arch			\
	-I$(VDEPTH)/lib/base 			\
	-I$(VDEPTH)/lib/mt	 			\
	-I$(VDEPTH)/lib/io	 			\
	-I$(VDEPTH)/lib/net	 			\
	-I$(VDEPTH)/lib/synergy			\
	-I$(VDEPTH)/lib/platform 		\
	$(NULL)

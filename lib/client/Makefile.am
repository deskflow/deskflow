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
	client.dsp						\
	CMSWindowsSecondaryScreen.cpp	\
	CMSWindowsSecondaryScreen.h		\
	$(NULL)

MAINTAINERCLEANFILES =				\
	Makefile.in						\
	$(NULL)

noinst_LIBRARIES = libclient.a
libclient_a_SOURCES = 				\
	CClient.cpp						\
	CSecondaryScreen.cpp			\
	CServerProxy.cpp				\
	CXWindowsSecondaryScreen.cpp	\
	CClient.h						\
	CSecondaryScreen.h				\
	CServerProxy.h					\
	CXWindowsSecondaryScreen.h		\
	ISecondaryScreenFactory.h		\
	$(NULL)
INCLUDES =							\
	-I$(VDEPTH)/lib/base 			\
	-I$(VDEPTH)/lib/mt	 			\
	-I$(VDEPTH)/lib/io	 			\
	-I$(VDEPTH)/lib/net	 			\
	-I$(VDEPTH)/lib/synergy			\
	-I$(VDEPTH)/lib/platform 		\
	$(NULL)

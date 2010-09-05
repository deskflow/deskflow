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

EXTRA_DIST =				\
	mt.dsp					\
	$(NULL)

MAINTAINERCLEANFILES =		\
	Makefile.in				\
	$(NULL)

noinst_LIBRARIES = libmt.a
libmt_a_SOURCES = 			\
	CCondVar.cpp			\
	CLock.cpp				\
	CMutex.cpp				\
	CThread.cpp				\
	XMT.cpp					\
	CCondVar.h				\
	CLock.h					\
	CMutex.h				\
	CThread.h				\
	XMT.h					\
	XThread.h				\
	$(NULL)
INCLUDES =						\
	-I$(top_srcdir)/lib/common	\
	-I$(top_srcdir)/lib/arch	\
	-I$(top_srcdir)/lib/base 	\
	$(NULL)

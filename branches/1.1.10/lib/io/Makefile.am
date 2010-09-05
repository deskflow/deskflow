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
	io.dsp						\
	$(NULL)

MAINTAINERCLEANFILES =			\
	Makefile.in					\
	$(NULL)

noinst_LIBRARIES = libio.a
libio_a_SOURCES = 				\
	CStreamBuffer.cpp			\
	CStreamFilter.cpp			\
	IStream.cpp					\
	XIO.cpp						\
	CStreamBuffer.h				\
	CStreamFilter.h				\
	IStream.h					\
	IStreamFilterFactory.h		\
	XIO.h						\
	$(NULL)
INCLUDES =						\
	-I$(top_srcdir)/lib/common	\
	-I$(top_srcdir)/lib/arch	\
	-I$(top_srcdir)/lib/base 	\
	-I$(top_srcdir)/lib/mt	 	\
	$(NULL)

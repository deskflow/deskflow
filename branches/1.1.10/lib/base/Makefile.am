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

EXTRA_DIST =				\
	base.dsp				\
	$(NULL)

MAINTAINERCLEANFILES =		\
	Makefile.in				\
	$(NULL)

noinst_LIBRARIES = libbase.a
libbase_a_SOURCES =			\
	CFunctionJob.cpp		\
	CJobList.cpp			\
	CLog.cpp				\
	CStopwatch.cpp			\
	CStringUtil.cpp			\
	CUnicode.cpp			\
	LogOutputters.cpp		\
	XBase.cpp				\
	CFunctionJob.h			\
	CJobList.h				\
	CLog.h					\
	CStopwatch.h			\
	CString.h				\
	CStringUtil.h			\
	CUnicode.h				\
	IJob.h					\
	ILogOutputter.h			\
	LogOutputters.h			\
	TMethodJob.h			\
	XBase.h					\
	$(NULL)
INCLUDES =					\
	-I$(VDEPTH)/lib/common	\
	-I$(VDEPTH)/lib/arch	\
	$(NULL)

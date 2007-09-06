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
	Makefile.win				\
	$(NULL)

MAINTAINERCLEANFILES =			\
	Makefile.in					\
	$(NULL)

noinst_LIBRARIES = libbase.a
libbase_a_SOURCES =				\
	CEvent.cpp					\
	CEventQueue.cpp				\
	CFunctionEventJob.cpp		\
	CFunctionJob.cpp			\
	CLog.cpp					\
	CSimpleEventQueueBuffer.cpp	\
	CStopwatch.cpp				\
	CStringUtil.cpp				\
	CUnicode.cpp				\
	IEventQueue.cpp				\
	LogOutputters.cpp			\
	XBase.cpp					\
	CEvent.h					\
	CEventQueue.h				\
	CFunctionEventJob.h			\
	CFunctionJob.h				\
	CLog.h						\
	CPriorityQueue.h			\
	CSimpleEventQueueBuffer.h	\
	CStopwatch.h				\
	CString.h					\
	CStringUtil.h				\
	CUnicode.h					\
	IEventJob.h					\
	IEventQueue.h				\
	IEventQueueBuffer.h			\
	IJob.h						\
	ILogOutputter.h				\
	LogOutputters.h				\
	TMethodEventJob.h			\
	TMethodJob.h				\
	XBase.h						\
	$(NULL)
INCLUDES =						\
	-I$(top_srcdir)/lib/common	\
	-I$(top_srcdir)/lib/arch	\
	$(NULL)

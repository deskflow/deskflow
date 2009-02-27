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

COMMON_SOURCE_FILES =			\
	CArch.cpp					\
	CArchDaemonNone.cpp			\
	CArchDaemonNone.h			\
	XArch.cpp					\
	CArch.h						\
	IArchConsole.h				\
	IArchDaemon.h				\
	IArchFile.h					\
	IArchLog.h					\
	IArchMultithread.h			\
	IArchNetwork.h				\
	IArchSleep.h				\
	IArchString.h				\
	IArchSystem.h				\
	IArchTaskBar.h				\
	IArchTaskBarReceiver.h		\
	IArchTime.h					\
	XArch.h						\
	$(NULL)
UNIX_SOURCE_FILES =				\
	CArchConsoleUnix.cpp		\
	CArchDaemonUnix.cpp			\
	CArchFileUnix.cpp			\
	CArchLogUnix.cpp			\
	CArchMultithreadPosix.cpp	\
	CArchNetworkBSD.cpp			\
	CArchSleepUnix.cpp			\
	CArchStringUnix.cpp			\
	CArchSystemUnix.cpp			\
	CArchTaskBarXWindows.cpp	\
	CArchTimeUnix.cpp			\
	XArchUnix.cpp				\
	CArchConsoleUnix.h			\
	CArchDaemonUnix.h			\
	CArchFileUnix.h				\
	CArchLogUnix.h				\
	CArchMultithreadPosix.h		\
	CArchNetworkBSD.h			\
	CArchSleepUnix.h			\
	CArchStringUnix.h			\
	CArchSystemUnix.h			\
	CArchTaskBarXWindows.h		\
	CArchTimeUnix.h				\
	XArchUnix.h					\
	$(NULL)
WIN32_SOURCE_FILES = 			\
	CArchConsoleWindows.cpp		\
	CArchDaemonWindows.cpp		\
	CArchFileWindows.cpp		\
	CArchLogWindows.cpp			\
	CArchMiscWindows.cpp		\
	CArchMultithreadWindows.cpp	\
	CArchNetworkWinsock.cpp		\
	CArchSleepWindows.cpp		\
	CArchStringWindows.cpp		\
	CArchSystemWindows.cpp		\
	CArchTaskBarWindows.cpp		\
	CArchTimeWindows.cpp		\
	XArchWindows.cpp			\
	CArchConsoleWindows.h		\
	CArchDaemonWindows.h		\
	CArchFileWindows.h			\
	CArchLogWindows.h			\
	CArchMiscWindows.h			\
	CArchMultithreadWindows.h	\
	CArchNetworkWinsock.h		\
	CArchSleepWindows.h			\
	CArchStringWindows.h		\
	CArchSystemWindows.h		\
	CArchTaskBarWindows.h		\
	CArchTimeWindows.h			\
	XArchWindows.h				\
	$(NULL)

EXTRA_DIST =					\
	CMultibyte.cpp				\
	Makefile.win				\
	vsnprintf.cpp				\
	$(UNIX_SOURCE_FILES)		\
	$(WIN32_SOURCE_FILES)		\
	$(NULL)

MAINTAINERCLEANFILES =			\
	Makefile.in					\
	$(NULL)

noinst_LIBRARIES = libarch.a
if UNIX
libarch_a_SOURCES =				\
	$(COMMON_SOURCE_FILES)		\
	$(UNIX_SOURCE_FILES)		\
	$(NULL)
endif
if WIN32
libarch_a_SOURCES =				\
	$(COMMON_SOURCE_FILES)		\
	$(WIN32_SOURCE_FILES)		\
	$(NULL)
endif
INCLUDES =						\
	-I$(top_srcdir)/lib/common	\
	$(NULL)

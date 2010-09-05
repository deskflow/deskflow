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

EXTRA_DIST =					\
	arch.dsp					\
	CArchConsoleWindows.cpp		\
	CArchDaemonWindows.cpp		\
	CArchFileWindows.cpp		\
	CArchLogWindows.cpp			\
	CArchMiscWindows.cpp		\
	CArchMultithreadWindows.cpp	\
	CArchNetworkWinsock.cpp		\
	CArchSleepWindows.cpp		\
	CArchStringWindows.cpp		\
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
	CArchTaskBarWindows.h		\
	CArchTimeWindows.h			\
	XArchWindows.h				\
	$(NULL)

MAINTAINERCLEANFILES =			\
	Makefile.in					\
	$(NULL)

noinst_LIBRARIES = libarch.a
libarch_a_SOURCES =				\
	CArch.cpp					\
	CArchImpl.cpp				\
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
	IArchTaskBar.h				\
	IArchTaskBarReceiver.h		\
	IArchTime.h					\
	XArch.h						\
	$(NULL)
EXTRA_libarch_a_SOURCES =		\
	CArchConsoleUnix.cpp		\
	CArchDaemonNone.cpp			\
	CArchDaemonUnix.cpp			\
	CArchFileUnix.cpp			\
	CArchLogUnix.cpp			\
	CArchMultithreadPosix.cpp	\
	CArchNetworkBSD.cpp			\
	CArchSleepUnix.cpp			\
	CArchStringUnix.cpp			\
	CArchTaskBarXWindows.cpp	\
	CArchTimeUnix.cpp			\
	CMultibyte.cpp				\
	CMultibyteOS.cpp			\
	CMultibyteEmu.cpp			\
	XArchUnix.cpp				\
	vsnprintf.cpp				\
	CArchConsoleUnix.h			\
	CArchDaemonNone.h			\
	CArchDaemonUnix.h			\
	CArchFileUnix.h				\
	CArchLogUnix.h				\
	CArchMultithreadPosix.h		\
	CArchNetworkBSD.h			\
	CArchSleepUnix.h			\
	CArchStringUnix.h			\
	CArchTaskBarXWindows.h		\
	CArchTimeUnix.h				\
	XArchUnix.h					\
	$(NULL)
INCLUDES =						\
	-I$(VDEPTH)/lib/common		\
	$(NULL)

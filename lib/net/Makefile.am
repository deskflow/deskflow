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

EXTRA_DIST =						\
	net.dsp							\
	$(NULL)

MAINTAINERCLEANFILES =				\
	Makefile.in						\
	$(NULL)

noinst_LIBRARIES = libnet.a
libnet_a_SOURCES = 					\
	CNetworkAddress.cpp				\
	CSocketMultiplexer.cpp			\
	CTCPListenSocket.cpp			\
	CTCPSocket.cpp					\
	CTCPSocketFactory.cpp			\
	IDataSocket.cpp					\
	IListenSocket.cpp				\
	ISocket.cpp						\
	XSocket.cpp						\
	CNetworkAddress.h				\
	CSocketMultiplexer.h			\
	CTCPListenSocket.h				\
	CTCPSocket.h					\
	CTCPSocketFactory.h				\
	IDataSocket.h					\
	IListenSocket.h					\
	ISocket.h						\
	ISocketFactory.h				\
	ISocketMultiplexerJob.h			\
	TSocketMultiplexerMethodJob.h	\
	XSocket.h						\
	$(NULL)
INCLUDES =							\
	-I$(top_srcdir)/lib/common		\
	-I$(top_srcdir)/lib/arch		\
	-I$(top_srcdir)/lib/base 		\
	-I$(top_srcdir)/lib/mt	 		\
	-I$(top_srcdir)/lib/io	 		\
	$(NULL)

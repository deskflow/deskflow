# synergy -- mouse and keyboard sharing utility
# Copyright (C) 2009 Chris Schoeneman, Nick Bolton, Sorin Sbarnea
# 
# This package is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# found in the file COPYING that should have accompanied this file.
# 
# This package is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

set(inc
	CNetworkAddress.h
	CSocketMultiplexer.h
	CTCPListenSocket.h
	CTCPSocket.h
	CTCPSocketFactory.h
	IDataSocket.h
	IListenSocket.h
	ISocket.h
	ISocketFactory.h
	ISocketMultiplexerJob.h
	TSocketMultiplexerMethodJob.h
	XSocket.h
)

set(src
	CNetworkAddress.cpp
	CSocketMultiplexer.cpp
	CTCPListenSocket.cpp
	CTCPSocket.cpp
	CTCPSocketFactory.cpp
	IDataSocket.cpp
	IListenSocket.cpp
	ISocket.cpp
	XSocket.cpp
)

if (WIN32)
	list(APPEND src ${inc})
endif()

set(inc
	../arch
	../base
	../common
	../io
	../mt
	../synergy
)

if (UNIX)
	list(APPEND inc
		../../..
	)
endif()

include_directories(${inc})
add_library(net STATIC ${src})

if (UNIX)
	target_link_libraries(net mt io)
endif()

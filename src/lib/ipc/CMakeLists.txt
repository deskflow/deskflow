# synergy -- mouse and keyboard sharing utility
# Copyright (C) 2012 Bolton Software Ltd.
# Copyright (C) 2009 Nick Bolton
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
	Ipc.h
	CIpcServer.h
	CIpcClient.h
	CIpcServerProxy.h
	CIpcClientProxy.h
	CIpcMessage.h
	CIpcLogOutputter.h
)

set(src
	Ipc.cpp
	CIpcServer.cpp
	CIpcClient.cpp
	CIpcServerProxy.cpp
	CIpcClientProxy.cpp
	CIpcMessage.cpp
	CIpcLogOutputter.cpp
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
	../net
	../synergy
)

if (UNIX)
	list(APPEND inc
		../../..
	)
endif()

include_directories(${inc})
add_library(ipc STATIC ${src})

if (UNIX)
	target_link_libraries(ipc arch base common mt io net synergy)
endif()

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
	CBaseClientProxy.h
	CClientListener.h
	CClientProxy.h
	CClientProxy1_0.h
	CClientProxy1_1.h
	CClientProxy1_2.h
	CClientProxy1_3.h
	CClientProxy1_4.h
	CClientProxyUnknown.h
	CConfig.h
	CInputFilter.h
	CPrimaryClient.h
	CServer.h
)

set(src
	CBaseClientProxy.cpp
	CClientListener.cpp
	CClientProxy.cpp
	CClientProxy1_0.cpp
	CClientProxy1_1.cpp
	CClientProxy1_2.cpp
	CClientProxy1_3.cpp
	CClientProxy1_4.cpp
	CClientProxyUnknown.cpp
	CConfig.cpp
	CInputFilter.cpp
	CPrimaryClient.cpp
	CServer.cpp
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
	../../../tools
)

if (UNIX)
	list(APPEND inc
		../../..
	)
endif()

include_directories(${inc})
add_library(server STATIC ${src})

if (WIN32)
	if (GAME_DEVICE_SUPPORT)
		include_directories($ENV{DXSDK_DIR}/Include)
	endif()
endif()

if (UNIX)
	target_link_libraries(server synergy)
endif()

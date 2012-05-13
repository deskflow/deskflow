# synergy -- mouse and keyboard sharing utility
# Copyright (C) 2012 Nick Bolton
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
	AddNewClientDialog.h
	JavaViewer.h
	ManagedListener.h
	QueryConnectDialog.h
	resource.h
	STrayIcon.h
	VNCServerService.h
	VNCServerWin32.h
	winvnc.h
)

set(src
	buildTime.cxx
	JavaViewer.cxx
	ManagedListener.cxx
	QueryConnectDialog.cxx
	STrayIcon.cxx
	VNCServerService.cxx
	VNCServerWin32.cxx
	winvnc.cxx
	connected.ico
	winvnc.ico
	winvnc.bmp
	winvnc.rc
)

if (WIN32)
	list(APPEND src ${inc})
endif()

set(inc
	..
	../../common
)

include_directories(${inc})
add_library(vnc_server STATIC ${src})
target_link_libraries(vnc_server
	vnc_rfb vnc_rfb_win32 vnc_network)

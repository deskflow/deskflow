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
	CConn.h
	CConnOptions.h
	CConnThread.h
	ConnectingDialog.h
	ConnectionDialog.h
	DesktopWindow.h
	InfoDialog.h
	ListenServer.h
	ListenTrayIcon.h
	MRU.h
	OptionsDialog.h
	resource.h
	UserPasswdDialog.h
	vncviewer.h
)

set(src
	buildTime.cxx
	CConn.cxx
	CConnOptions.cxx
	CConnThread.cxx
	ConnectingDialog.cxx
	ConnectionDialog.cxx
	DesktopWindow.cxx
	InfoDialog.cxx
	OptionsDialog.cxx
	UserPasswdDialog.cxx
	vncviewer.cxx
)

if (WIN32)
	list(APPEND src ${inc})
endif()

set(inc
	..
	../../common
)

include_directories(${inc})
add_library(vnc_client STATIC ${src})

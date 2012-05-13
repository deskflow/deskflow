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
	AboutDialog.h
	BitmapInfo.h
	CKeyboard.h
	CleanDesktop.h
	Clipboard.h
	CompatibleBitmap.h
	ComputerName.h
	CPointer.h
	CurrentUser.h
	DeviceContext.h
	DeviceFrameBuffer.h
	Dialog.h
	DIBSectionBuffer.h
	DynamicFn.h
	EventManager.h
	Handle.h
	IconInfo.h
	IntervalTimer.h
	keymap.h
	LaunchProcess.h
	LocalMem.h
	LogicalPalette.h
	LowLevelKeyEvents.h
	ModuleFileName.h
	MonitorInfo.h
	MsgBox.h
	MsgWindow.h
	OSVersion.h
	RegConfig.h
	Registry.h
	SDisplay.h
	SDisplayCoreDriver.h
	SDisplayCorePolling.h
	SDisplayCoreWMHooks.h
	Security.h
	Service.h
	SInput.h
	SocketManager.h
	TCharArray.h
	Threading.h
	TrayIcon.h
	TsSessions.h
	Win32Util.h
	WMCursor.h
	WMNotifier.h
	WMPoller.h
	WMShatter.h
	WMWindowCopyRect.h
	WMHooks.h
)

set(src
	AboutDialog.cxx
	CKeyboard.cxx
	CleanDesktop.cxx
	Clipboard.cxx
	CPointer.cxx
	CurrentUser.cxx
	DeviceContext.cxx
	DeviceFrameBuffer.cxx
	Dialog.cxx
	DIBSectionBuffer.cxx
	DynamicFn.cxx
	EventManager.cxx
	LaunchProcess.cxx
	LowLevelKeyEvents.cxx
	MonitorInfo.cxx
	MsgWindow.cxx
	OSVersion.cxx
	RegConfig.cxx
	Registry.cxx
	SDisplay.cxx
	SDisplayCorePolling.cxx
	SDisplayCoreWMHooks.cxx
	Security.cxx
	Service.cxx
	SInput.cxx
	SocketManager.cxx
	TCharArray.cxx
	Threading.cxx
	TsSessions.cxx
	Win32Util.cxx
	WMCursor.cxx
	WMNotifier.cxx
	WMPoller.cxx
	WMShatter.cxx
	WMWindowCopyRect.cxx
	WMHooks.cxx
)

if (WIN32)
	list(APPEND src ${inc})
endif()

set(inc
	..
	../../common
)

include_directories(${inc})
add_library(vnc_rfb_win32 STATIC ${src})
target_link_libraries(vnc_rfb_win32
	vnc_rdr ws2_32 version comctl32)

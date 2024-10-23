# Deskflow -- mouse and keyboard sharing utility
# Copyright (C) 2024 Chris Rizzitello <sithlord48@gmail.com>
# Copyright (C) 2012-2024 Symless Ltd.
# Copyright (C) 2009-2012 Nick Bolton
#
# This package is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# found in the file LICENSE that should have accompanied this file.
#
# This package is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

add_library(ipc STATIC
  IpcClient.cpp
  IpcClient.h
  IpcClientProxy.cpp
  IpcClientProxy.h
  IpcLogOutputter.cpp
  IpcLogOutputter.h
  IpcMessage.cpp
  IpcMessage.h
  IpcServer.cpp
  IpcServer.h
  IpcServerProxy.cpp
  IpcServerProxy.h
  IpcSettingMessage.cpp
  IpcSettingMessage.h
)

if(UNIX)
  target_link_libraries(
    ipc
    arch
    base
    mt
    io
    net
    app)
endif()

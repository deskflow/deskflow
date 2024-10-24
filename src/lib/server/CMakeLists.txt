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

add_library(server STATIC
  BaseClientProxy.cpp
  BaseClientProxy.h
  ClientListener.cpp
  ClientListener.h
  ClientProxy.cpp
  ClientProxy.h
  ClientProxy1_0.cpp
  ClientProxy1_0.h
  ClientProxy1_1.cpp
  ClientProxy1_1.h
  ClientProxy1_2.cpp
  ClientProxy1_2.h
  ClientProxy1_3.cpp
  ClientProxy1_3.h
  ClientProxy1_4.cpp
  ClientProxy1_4.h
  ClientProxy1_5.cpp
  ClientProxy1_5.h
  ClientProxy1_6.cpp
  ClientProxy1_6.h
  ClientProxy1_7.cpp
  ClientProxy1_7.h
  ClientProxy1_8.cpp
  ClientProxy1_8.h
  ClientProxyUnknown.cpp
  ClientProxyUnknown.h
  Config.cpp
  Config.h
  InputFilter.cpp
  InputFilter.h
  PrimaryClient.cpp
  PrimaryClient.h
  Server.cpp
  Server.h
)

if(UNIX)
  target_link_libraries(server app)
endif()

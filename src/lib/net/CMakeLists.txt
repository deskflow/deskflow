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

add_library(net STATIC
  IDataSocket.cpp
  IDataSocket.h
  IListenSocket.h
  ISocket.h
  ISocketFactory.h
  ISocketMultiplexerJob.h
  NetworkAddress.cpp
  NetworkAddress.h
  SecureListenSocket.cpp
  SecureListenSocket.h
  SecurityLevel.h
  SecureSocket.cpp
  SecureSocket.h
  SocketMultiplexer.cpp
  SocketMultiplexer.h
  TCPListenSocket.cpp
  TCPListenSocket.h
  TCPSocket.cpp
  TCPSocket.h
  TCPSocketFactory.cpp
  TCPSocketFactory.h
  TSocketMultiplexerMethodJob.h
  XSocket.cpp
  XSocket.h
  InverseSockets/AutoArchSocket.cpp
  InverseSockets/AutoArchSocket.h
  InverseSockets/InverseClientSocket.cpp
  InverseSockets/InverseClientSocket.h
  InverseSockets/InverseServerSocket.cpp
  InverseSockets/InverseServerSocket.h
  InverseSockets/InverseSocketFactory.cpp
  InverseSockets/InverseSocketFactory.h
  InverseSockets/SecureClientSocket.cpp
  InverseSockets/SecureClientSocket.h
  InverseSockets/SecureServerSocket.cpp
  InverseSockets/SecureServerSocket.h
  InverseSockets/SslApi.cpp
  InverseSockets/SslApi.h
  InverseSockets/SslLogger.cpp
  InverseSockets/SslLogger.h
)

target_link_libraries(
  net
  PUBLIC OpenSSL::SSL OpenSSL::Crypto
  PRIVATE mt io)

if(WIN32)
  target_link_libraries(net PRIVATE Crypt32 ws2_32)
endif()

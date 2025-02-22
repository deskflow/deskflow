# SPDX-FileCopyrightText: 2024 - 2025 Chris Rizzitello <sithlord48@gmail.com>
# SPDX-FileCopyrightText: 2012 - 2024 Symless Ltd
# SPDX-FileCopyrightText: 2009 - 2012 Nick Bolton
# SPDX-License-Identifier: MIT

add_library(net STATIC
  FingerprintData.cpp
  FingerprintData.h
  FingerprintDatabase.cpp
  FingerprintDatabase.h
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
  SecureUtils.cpp
  SecureUtils.h
  SslLogger.cpp
  SslLogger.h
  TCPListenSocket.cpp
  TCPListenSocket.h
  TCPSocket.cpp
  TCPSocket.h
  TCPSocketFactory.cpp
  TCPSocketFactory.h
  TSocketMultiplexerMethodJob.h
  XSocket.cpp
  XSocket.h
)

target_link_libraries(
  net
  PUBLIC OpenSSL::SSL OpenSSL::Crypto
  PRIVATE mt io)

if(WIN32)
  target_link_libraries(
    net
    PUBLIC OpenSSL::applink
    PRIVATE Crypt32 ws2_32 OpenSSL::applink
  )
endif()

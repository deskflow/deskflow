# SPDX-FileCopyrightText: 2024 - 2025 Chris Rizzitello <sithlord48@gmail.com>
# SPDX-FileCopyrightText: 2012 - 2024 Symless Ltd
# SPDX-FileCopyrightText: 2009 - 2012 Nick Bolton
# SPDX-License-Identifier: MIT

# Apple has to use static libraries because "Use of the Apple-provided OpenSSL
# libraries by apps is strongly discouraged."
# https://developer.apple.com/library/archive/documentation/Security/Conceptual/cryptoservices/SecureNetworkCommunicationAPIs/SecureNetworkCommunicationAPIs.html
if(APPLE)
  set(OPENSSL_USE_STATIC_LIBS TRUE)
endif()

find_package(OpenSSL ${REQUIRED_OPENSSL_VERSION} REQUIRED COMPONENTS SSL Crypto)

add_library(net STATIC
  Fingerprint.cpp
  Fingerprint.h
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
  PUBLIC OpenSSL::SSL OpenSSL::Crypto common
  PRIVATE mt io)

if(WIN32)
  target_link_libraries(
    net
    PUBLIC OpenSSL::applink
    PRIVATE Crypt32 ws2_32
  )
endif()

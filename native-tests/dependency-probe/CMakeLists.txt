# SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
# SPDX-License-Identifier: MIT
cmake_minimum_required(VERSION 3.24)
project(DeskflowDependencyProbe LANGUAGES CXX)
set(OPENSSL_USE_STATIC_LIBS TRUE)
find_package(OpenSSL 3.0 QUIET COMPONENTS SSL Crypto)
find_package(Qt6 6.7 QUIET COMPONENTS Core Widgets Network OPTIONAL_COMPONENTS LinguistTools Svg)
find_program(PROBE_LRELEASE lrelease HINTS "${CMAKE_PREFIX_PATH}/bin" "${CMAKE_PREFIX_PATH}/libexec")
find_program(PROBE_LUPDATE lupdate HINTS "${CMAKE_PREFIX_PATH}/bin" "${CMAKE_PREFIX_PATH}/libexec")
find_program(PROBE_MACDEPLOYQT macdeployqt HINTS "${CMAKE_PREFIX_PATH}/bin")
find_file(PROBE_QT_ENGLISH qtbase_en.qm HINTS "${CMAKE_PREFIX_PATH}/translations")
foreach(var OPENSSL_FOUND OPENSSL_VERSION OPENSSL_INCLUDE_DIR OPENSSL_SSL_LIBRARY OPENSSL_CRYPTO_LIBRARY
    Qt6Core_FOUND Qt6Widgets_FOUND Qt6Network_FOUND Qt6LinguistTools_FOUND Qt6Svg_FOUND
    PROBE_LRELEASE PROBE_LUPDATE PROBE_MACDEPLOYQT PROBE_QT_ENGLISH)
  message(STATUS "dependency-probe ${var}=${${var}}")
endforeach()

# SPDX-FileCopyrightText: (C) 2024 - 2025 Chris Rizzitello <sithlord48@gmail.com>
# SPDX-License-Identifier: MIT

set(CORE_BINARY "${DESKFLOW_RUNTIME_ID}-core")
if(DESKFLOW_LAB_BUILD)
  set(CORE_BINARY "deskflow-lab-core")
endif()
if(WIN32)
  string(APPEND CORE_BINARY ".exe")
endif()

configure_file(Constants.h.in Constants.h @ONLY)
configure_file(VersionInfo.h.in VersionInfo.h @ONLY)

add_library(common STATIC
  DesktopWorkspace.cpp
  DesktopWorkspace.h
  DisplayTopology.cpp
  DisplayTopology.h
  NativeDisplayTopology.cpp
  CandidatePolicy.cpp
  CandidatePolicy.h
  RuntimeProfile.cpp
  RuntimeProfile.h
  WindowsCredentialFiles.cpp
  WindowsCredentialFiles.h
  CredentialStore.cpp
  CredentialStore.h
  IsolatedSession.cpp
  IsolatedSession.h
  Action.cpp
  Action.h
  Coordinate.h
  Enums.h
  ExitCodes.h
  Hotkey.cpp
  Hotkey.h
  I18N.h
  I18N.cpp
  KeyModifierID.h
  KeyboardModifier.h
  KeySequence.h
  KeySequence.cpp
  LogLevel.h
  NetworkProtocol.h
  PlatformInfo.h
  Settings.h
  Settings.cpp
  QSettingsProxy.cpp
  QSettingsProxy.h
  UrlConstants.h
  ${CMAKE_CURRENT_BINARY_DIR}/Constants.h
  ${CMAKE_CURRENT_BINARY_DIR}/VersionInfo.h
)

target_link_libraries(common PUBLIC Qt6::Core PRIVATE Qt6::Gui)
if(APPLE)
  target_link_libraries(common PRIVATE "-framework CoreGraphics" "-framework ColorSync")
elseif(WIN32)
  target_link_libraries(common PRIVATE user32)
endif()

target_include_directories(common
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/lib>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/src/lib>
)

if(WIN32)
  target_link_libraries(common PRIVATE advapi32)
endif()

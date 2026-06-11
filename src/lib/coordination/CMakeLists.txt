# SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
# SPDX-License-Identifier: MIT

add_library(coordination STATIC
  CoordinationMesh.cpp
  CoordinationMesh.h
  CoordinationProtocol.cpp
  CoordinationProtocol.h
  Coordinator.cpp
  Coordinator.h
  ElectionState.cpp
  ElectionState.h
  LocalInputMonitor.h
  Peer.cpp
  Peer.h
)

if(APPLE)
  target_sources(coordination PRIVATE OSXLocalInputMonitor.mm)
  target_link_libraries(coordination PUBLIC "-framework ApplicationServices")
elseif(WIN32)
  target_sources(coordination PRIVATE MSWindowsLocalInputMonitor.cpp)
  target_link_libraries(coordination PUBLIC ws2_32)
else()
  target_sources(coordination PRIVATE StubLocalInputMonitor.cpp)
endif()

target_link_libraries(coordination PUBLIC common base)

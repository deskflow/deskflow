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
  FleetState.h
  FleetStateMerge.cpp
  FleetStateMerge.h
  LocalInputMonitor.h
  Peer.cpp
  Peer.h
)

if(APPLE)
  target_sources(coordination PRIVATE
    OSXLocalInputMonitor.mm
    OSXKeyboardRelayMonitor.mm
    KeyboardRelayMap.mm
  )
  target_link_libraries(coordination PUBLIC "-framework ApplicationServices" "-framework Carbon")
elseif(WIN32)
  target_sources(coordination PRIVATE
    MSWindowsLocalInputMonitor.cpp
    MSWindowsKeyboardRelayMonitor.cpp
    KeyboardRelayMap.cpp
  )
  target_link_libraries(coordination PUBLIC ws2_32)
else()
  target_sources(coordination PRIVATE
    StubLocalInputMonitor.cpp
    StubKeyboardRelayMonitor.cpp
  )
endif()

target_link_libraries(coordination PUBLIC common base)

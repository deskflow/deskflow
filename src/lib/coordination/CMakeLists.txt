# SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
# SPDX-License-Identifier: MIT

add_library(coordination STATIC
  CoordinationProtocol.cpp
  CoordinationProtocol.h
  ElectionState.cpp
  ElectionState.h
  Peer.cpp
  Peer.h
)

target_link_libraries(coordination PUBLIC common)

# SPDX-FileCopyrightText: 2024 Chris Rizzitello <sithlord48@gmail.com>
# SPDX-FileCopyrightText: 2012 - 2024 Symless Ltd
# SPDX-FileCopyrightText: 2009 - 2012 Nick Bolton
# SPDX-License-Identifier: MIT

add_library(base STATIC
  BaseException.cpp
  BaseException.h
  DirectionTypes.h
  Event.h
  EventQueue.cpp
  EventQueue.h
  EventQueueTimer.h
  EventTypes.h
  FinalAction.h
  FunctionEventJob.cpp
  FunctionEventJob.h
  FunctionJob.cpp
  FunctionJob.h
  IEventQueue.h
  IEventQueueBuffer.h
  IJob.h
  ILogOutputter.h
  LogOutputters.cpp
  LogOutputters.h
  Log.cpp
  Log.h
  LogLevel.h
  NetworkProtocol.h
  Path.cpp
  Path.h
  PriorityQueue.h
  SimpleEventQueueBuffer.cpp
  SimpleEventQueueBuffer.h
  Stopwatch.cpp
  Stopwatch.h
  String.cpp
  String.h
  TMethodJob.h
  Unicode.cpp
  Unicode.h
)

target_link_libraries(base PUBLIC arch)
if (CMAKE_CXX_BYTE_ORDER STREQUAL "BIG_ENDIAN")
  target_compile_definitions(base PUBLIC WORDS_BIGENDIAN=1)
endif()

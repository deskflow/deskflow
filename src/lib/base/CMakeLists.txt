# SPDX-FileCopyrightText: 2024 Chris Rizzitello <sithlord48@gmail.com>
# SPDX-FileCopyrightText: 2012 - 2024 Symless Ltd
# SPDX-FileCopyrightText: 2009 - 2012 Nick Bolton
# SPDX-License-Identifier: MIT

add_library(base STATIC
  ELevel.h
  Event.cpp
  Event.h
  EventQueue.cpp
  EventQueue.h
  EventTypes.cpp
  EventTypes.h
  finally.h
  FunctionEventJob.cpp
  FunctionEventJob.h
  FunctionJob.cpp
  FunctionJob.h
  IEventJob.h
  IEventQueue.h
  IEventQueueBuffer.h
  IJob.h
  ILogOutputter.h
  log_outputters.cpp
  log_outputters.h
  Log.cpp
  Log.h
  Path.cpp
  Path.h
  PriorityQueue.h
  SimpleEventQueueBuffer.cpp
  SimpleEventQueueBuffer.h
  Stopwatch.cpp
  Stopwatch.h
  String.cpp
  String.h
  TMethodEventJob.h
  TMethodJob.h
  Unicode.cpp
  Unicode.h
  XBase.cpp
  XBase.h
)


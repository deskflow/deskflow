# SPDX-FileCopyrightText: 2024 Chris Rizzitello <sithlord48@gmail.com>
# SPDX-FileCopyrightText: 2012 - 2024 Symless Ltd
# SPDX-FileCopyrightText: 2009 - 2012 Nick Bolton
# SPDX-License-Identifier: MIT

add_library(mt STATIC
  CondVar.cpp
  CondVar.h
  Lock.cpp
  Lock.h
  Mutex.cpp
  Mutex.h
  Thread.cpp
  Thread.h
  XMT.cpp
  XMT.h
  XThread.h
)

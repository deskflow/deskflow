# Deskflow -- mouse and keyboard sharing utility
# Copyright (C) 2024 Chris Rizzitello <sithlord48@gmail.com>
# Copyright (C) 2012-2024 Symless Ltd.
# Copyright (C) 2009-2012 Nick Bolton
#
# This package is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# found in the file LICENSE that should have accompanied this file.
#
# This package is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

add_library(base STATIC
  ELevel.h
  Event.cpp
  Event.h
  EventQueue.cpp
  EventQueue.h
  EventTypes.cpp
  EventTypes.h
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


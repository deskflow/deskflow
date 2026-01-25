/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

//! Log levels
/*!
The logging priority levels in order of highest to lowest priority.
*/
enum class LogLevel
{
  IPC = -2,   //!< For IPC Messages the gui must see, remove when IPC works proper
  Print = -1, //!< For print only (no file or time)
  Fatal,      //!< For fatal errors
  Error,      //!< For serious errors
  Warning,    //!< For minor errors and warnings
  Note,       //!< For messages about notable events
  Info,       //!< For informational messages
  Debug,      //!< For important debugging messages
  Debug1,     //!< For verbosity +1 debugging messages
  Debug2      //!< For verbosity +2 debugging messages
};

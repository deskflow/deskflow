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
enum ELevel
{
  kPRINT = -1, //!< For print only (no file or time)
  kFATAL,      //!< For fatal errors
  kERROR,      //!< For serious errors
  kWARNING,    //!< For minor errors and warnings
  kNOTE,       //!< For messages about notable events
  kINFO,       //!< For informational messages
  kDEBUG,      //!< For important debugging messages
  kDEBUG1,     //!< For verbosity +1 debugging messages
  kDEBUG2,     //!< For verbosity +2 debugging messages
  kDEBUG3,     //!< For verbosity +3 debugging messages
  kDEBUG4,     //!< For verbosity +4 debugging messages
  kDEBUG5      //!< For verbosity +5 debugging messages
};

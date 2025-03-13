/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "common/IInterface.h"
#include <string>

//! Interface for architecture dependent system queries
/*!
This interface defines operations for querying system info.
*/
class IArchSystem : public IInterface
{
public:
  //! @name accessors
  //@{

  //! Get a Deskflow setting
  /*!
  Reads a Deskflow setting from the system.
  */
  virtual std::string setting(const std::string &valueName) const = 0;
  //@}

  //! Set a Deskflow setting
  /*!
  Writes a Deskflow setting from the system.
  */
  virtual void setting(const std::string &valueName, const std::string &valueString) const = 0;
  //@}

  //! Delete settings
  /*!
  Deletes all Core settings from the system.
  */
  virtual void clearSettings() const = 0;
  //@}
};

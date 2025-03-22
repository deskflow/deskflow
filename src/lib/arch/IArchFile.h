/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "common/IInterface.h"
#include <string>

//! Interface for architecture dependent file system operations
/*!
This interface defines the file system operations required by
deskflow.  Each architecture must implement this interface.
*/
class IArchFile : public IInterface
{
public:
  //! @name manipulators
  //@{

  //! Get installed directory
  /*!
  Returns the directory in which Deskflow is installed.
  */
  virtual std::string getInstalledDirectory() = 0;
};

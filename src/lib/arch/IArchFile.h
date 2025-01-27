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

  //! Extract base name
  /*!
  Find the base name in the given \c pathname.
  */
  virtual const char *getBasename(const char *pathname) = 0;

  //! Get user's home directory
  /*!
  Returns the user's home directory.  Returns the empty string if
  this cannot be determined.
  */
  virtual std::string getUserDirectory() = 0;

  //! Get system directory
  /*!
  Returns the ussystem configuration file directory.
  */
  virtual std::string getSystemDirectory() = 0;

  //! Get installed directory
  /*!
  Returns the directory in which Deskflow is installed.
  */
  virtual std::string getInstalledDirectory() = 0;

  //! Get log directory
  /*!
  Returns the log file directory.
  */
  virtual std::string getLogDirectory() = 0;

  //! Get plugins directory
  /*!
  Returns the plugin files directory. If no plugin directory is set,
  this will return the plugin folder within the user's profile.
  */
  virtual std::string getPluginDirectory() = 0;

  //! Get user's profile directory
  /*!
  Returns the user's profile directory. If no profile directory is set,
  this will return the user's profile according to the operating system,
  which will depend on which user launched the program.
  */
  virtual std::string getProfileDirectory() = 0;

  //! Concatenate path components
  /*!
  Concatenate pathname components with a directory separator
  between them.  This should not check if the resulting path
  is longer than allowed by the system;  we'll rely on the
  system calls to tell us that.
  */
  virtual std::string concatPath(const std::string &prefix, const std::string &suffix) = 0;

  //@}
  //! Set the user's profile directory
  /*
  Returns the user's profile directory.
  */
  virtual void setProfileDirectory(const std::string &s) = 0;

  //@}
  //! Set the user's plugin directory
  /*
  Returns the user's plugin directory.
  */
  virtual void setPluginDirectory(const std::string &s) = 0;
};

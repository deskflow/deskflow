/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/Event.h"
#include "common/IInterface.h"

//! Screen saver interface
/*!
This interface defines the methods common to all screen savers.
*/
class IScreenSaver : public IInterface
{
public:
  // note -- the c'tor/d'tor must *not* enable/disable the screen saver

  //! @name manipulators
  //@{

  //! Enable screen saver
  /*!
  Enable the screen saver, restoring the screen saver settings to
  what they were when disable() was previously called.  If disable()
  wasn't previously called then it should keep the current settings
  or use reasonable defaults.
  */
  virtual void enable() = 0;

  //! Disable screen saver
  /*!
  Disable the screen saver, saving the old settings for the next
  call to enable().
  */
  virtual void disable() = 0;

  //! Activate screen saver
  /*!
  Activate (i.e. show) the screen saver.
  */
  virtual void activate() = 0;

  //! Deactivate screen saver
  /*!
  Deactivate (i.e. hide) the screen saver, reseting the screen saver
  timer.
  */
  virtual void deactivate() = 0;

  //@}
  //! @name accessors
  //@{

  //! Test if screen saver on
  /*!
  Returns true iff the screen saver is currently active (showing).
  */
  virtual bool isActive() const = 0;

  //@}
};

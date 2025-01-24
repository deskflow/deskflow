/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2003 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/Event.h"
#include "base/EventTypes.h"
#include "common/IInterface.h"
#include "deskflow/mouse_types.h"

//! Secondary screen interface
/*!
This interface defines the methods common to all platform dependent
secondary screen implementations.
*/
class ISecondaryScreen : public IInterface
{
public:
  //! @name accessors
  //@{

  //! Fake mouse press/release
  /*!
  Synthesize a press or release of mouse button \c id.
  */
  virtual void fakeMouseButton(ButtonID id, bool press) = 0;

  //! Fake mouse move
  /*!
  Synthesize a mouse move to the absolute coordinates \c x,y.
  */
  virtual void fakeMouseMove(int32_t x, int32_t y) = 0;

  //! Fake mouse move
  /*!
  Synthesize a mouse move to the relative coordinates \c dx,dy.
  */
  virtual void fakeMouseRelativeMove(int32_t dx, int32_t dy) const = 0;

  //! Fake mouse wheel
  /*!
  Synthesize a mouse wheel event of amount \c xDelta and \c yDelta.
  */
  virtual void fakeMouseWheel(int32_t xDelta, int32_t yDelta) const = 0;

  //@}
};

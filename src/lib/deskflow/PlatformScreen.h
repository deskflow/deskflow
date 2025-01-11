/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2004 Chris Schoeneman
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "common/stdexcept.h"
#include "deskflow/ClientArgs.h"
#include "deskflow/DragInformation.h"
#include "deskflow/IPlatformScreen.h"

//! Base screen implementation
/*!
This screen implementation is the superclass of all other screen
implementations.  It implements a handful of methods and requires
subclasses to implement the rest.
*/
class PlatformScreen : public IPlatformScreen
{
public:
  PlatformScreen(
      IEventQueue *events, deskflow::ClientScrollDirection scrollDirection = deskflow::ClientScrollDirection::SERVER
  );
  virtual ~PlatformScreen();

  // IScreen overrides
  virtual void *getEventTarget() const = 0;
  virtual bool getClipboard(ClipboardID id, IClipboard *) const = 0;
  virtual void getShape(int32_t &x, int32_t &y, int32_t &width, int32_t &height) const = 0;
  virtual void getCursorPos(int32_t &x, int32_t &y) const = 0;

  // IPrimaryScreen overrides
  virtual void reconfigure(UInt32 activeSides) = 0;
  virtual void warpCursor(int32_t x, int32_t y) = 0;
  virtual UInt32 registerHotKey(KeyID key, KeyModifierMask mask) = 0;
  virtual void unregisterHotKey(UInt32 id) = 0;
  virtual void fakeInputBegin() = 0;
  virtual void fakeInputEnd() = 0;
  virtual int32_t getJumpZoneSize() const = 0;
  virtual bool isAnyMouseButtonDown(UInt32 &buttonID) const = 0;
  virtual void getCursorCenter(int32_t &x, int32_t &y) const = 0;

  // ISecondaryScreen overrides
  virtual void fakeMouseButton(ButtonID id, bool press) = 0;
  virtual void fakeMouseMove(int32_t x, int32_t y) = 0;
  virtual void fakeMouseRelativeMove(int32_t dx, int32_t dy) const = 0;
  virtual void fakeMouseWheel(int32_t xDelta, int32_t yDelta) const = 0;

  // IKeyState overrides
  virtual void updateKeyMap();
  virtual void updateKeyState();
  virtual void setHalfDuplexMask(KeyModifierMask);
  virtual void fakeKeyDown(KeyID id, KeyModifierMask mask, KeyButton button, const std::string &);
  virtual bool fakeKeyRepeat(KeyID id, KeyModifierMask mask, int32_t count, KeyButton button, const std::string &lang);
  virtual bool fakeKeyUp(KeyButton button);
  virtual void fakeAllKeysUp();
  virtual bool fakeCtrlAltDel();
  virtual bool isKeyDown(KeyButton) const;
  virtual KeyModifierMask getActiveModifiers() const;
  virtual KeyModifierMask pollActiveModifiers() const;
  virtual int32_t pollActiveGroup() const;
  virtual void pollPressedKeys(KeyButtonSet &pressedKeys) const;

  virtual void setDraggingStarted(bool started)
  {
    m_draggingStarted = started;
  }
  virtual bool isDraggingStarted();
  virtual bool isFakeDraggingStarted()
  {
    return m_fakeDraggingStarted;
  }
  virtual std::string &getDraggingFilename()
  {
    return m_draggingFilename;
  }
  virtual void clearDraggingFilename()
  {
  }

  // IPlatformScreen overrides
  virtual void enable() = 0;
  virtual void disable() = 0;
  virtual void enter() = 0;
  virtual bool canLeave() = 0;
  virtual void leave() = 0;
  virtual bool setClipboard(ClipboardID, const IClipboard *) = 0;
  virtual void checkClipboards() = 0;
  virtual void openScreensaver(bool notify) = 0;
  virtual void closeScreensaver() = 0;
  virtual void screensaver(bool activate) = 0;
  virtual void resetOptions() = 0;
  virtual void setOptions(const OptionsList &options) = 0;
  virtual void setSequenceNumber(UInt32) = 0;
  virtual bool isPrimary() const = 0;

  virtual void fakeDraggingFiles(DragFileList fileList)
  {
    throw std::runtime_error("fakeDraggingFiles not implemented");
  }
  virtual const std::string &getDropTarget() const
  {
    throw std::runtime_error("getDropTarget not implemented");
  }

protected:
  //! Update mouse buttons
  /*!
  Subclasses must implement this method to update their internal mouse
  button mapping and, if desired, state tracking.
  */
  virtual void updateButtons() = 0;

  //! Get the key state
  /*!
  Subclasses must implement this method to return the platform specific
  key state object that each subclass must have.
  */
  virtual IKeyState *getKeyState() const = 0;

  // IPlatformScreen overrides
  virtual void handleSystemEvent(const Event &event, void *) = 0;

  /*!
   * \brief mapClientScrollDirection
   * Convert scroll according to client scroll directio
   * \return converted value according to the client scroll direction
   */
  virtual int32_t mapClientScrollDirection(int32_t) const;

protected:
  std::string m_draggingFilename;
  bool m_draggingStarted;
  bool m_fakeDraggingStarted;

private:
  /*!
   * \brief m_clientScrollDirection
   * This member contains client scroll direction.
   * This member is used only on client side.
   */
  deskflow::ClientScrollDirection m_clientScrollDirection = deskflow::ClientScrollDirection::SERVER;
};

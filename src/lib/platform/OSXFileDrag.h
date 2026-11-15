/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/FileDragTypes.h"

#include <cstdint>
#include <functional>
#include <memory>

class IEventQueue;

/**
 * @brief The macOS side of dragging files and folders between computers
 *
 * Plain C++ on the outside so OSXComputer.h stays free of Objective-C; the AppKit work is
 * in OSXFileDrag.mm. Every public method is called on the core thread (the thread that
 * runs the event queue). AppKit windows live on the main thread, where QApplication runs
 * the Cocoa event loop, and are reached through the main dispatch queue. Results come back
 * to the core thread as events, so OSXComputer, Server and Client never see another thread.
 *
 * Both roles work on either computer: the server sends files to a client and takes them
 * from one, and so does a client (the server tells it which edge to watch).
 *
 * Source: thin edge windows registered for file URLs notice a Finder drag of
 * files reaching a switch edge (a window move, Mission Control or a text selection is not
 * a drag of files and never reaches them), the items are read from the drag pasteboard
 * and sent with FileDragSender. While the cursor is on the other computer the local drag
 * waits over a "catcher" window that refuses it, so letting go never moves or copies
 * anything here.
 *
 * Destination: the items are staged with FileDragStaging; a small window
 * under the cursor gets a synthetic press and starts an AppKit drag of one
 * NSFilePromiseProvider per top-level item. Finder (or any promise receiver) names the
 * destination when the user lets go; the staged item is moved there under a free name
 * once it has fully arrived.
 */
class OSXFileDrag
{
public:
  //! What the drag needs from OSXComputer; called on the core thread
  struct Hooks
  {
    //! destination: the start window is under the cursor; press the left button at x,y
    std::function<void(uint32_t dragId, int32_t x, int32_t y)> launcherReady;
    //! destination: the native drag loop ended (dropped or not); let go of our press
    std::function<void(uint32_t dragId, bool dropped)> loopEnded;
    //! destination: end a running drag over the refusing window at x,y (move there, let go)
    std::function<void(int32_t x, int32_t y)> releaseAt;
    //! source: sending \p dragId stopped here (failed or nothing to send); a cancel went out
    std::function<void(uint32_t dragId)> sourceStopped;
  };

  /**
   * \p eventTarget is the platform computer's event target: FileDragEvent events for the
   * server or client are posted to it.
   */
  OSXFileDrag(IEventQueue *events, void *eventTarget, bool primary, Hooks hooks);
  ~OSXFileDrag();

  OSXFileDrag(const OSXFileDrag &) = delete;
  OSXFileDrag &operator=(const OSXFileDrag &) = delete;

  //! deskflow::filedrag::Capability flags (both roles; none without AppKit)
  uint32_t capabilities() const;

  //! @name source (files leave this computer)
  //@{

  //! Shows the edge windows on the sides in \p sideMask (DirectionMask) of the desktop
  void watchEdges(uint32_t sideMask, int32_t x, int32_t y, int32_t w, int32_t h);
  //! The drag offered at the edge continues on the other computer: park it at \p parkX, \p parkY and send it
  void handOff(uint32_t dragId, int32_t parkX, int32_t parkY);
  //! The cursor came back before the drop: stop sending, the local drag carries on
  void takeBack();
  //! The cursor is back on this computer: nothing stays parked
  void entered();
  /**
   * Event tap thread (server only): true (once) for the left button release that ends the
   * parked local drag. That release must reach the system, where the catcher refuses the drop.
   */
  bool takeParkedRelease();
  //@}

  //! A file drag message from the peer: acks, dropped, finished and cancels for drags sent
  //! from here; items, data and cancels for drags coming here (matched by drag id)
  void receive(const deskflow::filedrag::Message &message);

  struct State;

private:
  std::shared_ptr<State> m_state;
};

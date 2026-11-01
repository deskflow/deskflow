/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Lucas Magotteaux
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <ApplicationServices/ApplicationServices.h>

#include <optional>
#include <string>
#include <vector>

using IOHIDEventRef = struct __IOHIDEvent *;

//! Accessibility Zoom trackpad gesture across computers (macOS)
/*!
With "Use trackpad gesture to zoom" (System Settings > Accessibility > Zoom),
a three-finger double-tap toggles Zoom and a three-finger double-tap that
stays down and drags up or down changes the zoom level.

macOS reads that gesture from the local trackpad device. It cannot be
replayed on a computer without a trackpad: the double-tap still toggles
there, but the drag does nothing. So the primary recognizes the gesture
from its touch frames and sends Zoom commands; the secondary runs them
through its own Zoom engine, as NX_ZOOM events (the events macOS makes for
"Use scroll gesture with modifier keys to zoom"). Zoom handles those like the
trackpad drag, so the zoom style, smoothing and focus settings still apply.
*/
namespace deskflow::osx::zoom {

//! Fingers on the trackpad at one instant
struct TouchFrame
{
  double time = 0; //!< seconds, monotonic
  int fingers = 0; //!< fingers touching the surface
  double x = 0;    //!< centroid of touching fingers, mm from the left edge
  double y = 0;    //!< centroid of touching fingers, mm from the bottom edge
};

//! Size of a trackpad surface, in millimetres
struct Surface
{
  double width = 124.8;
  double height = 76.8;
};

//! The surface of the first trackpad found, or a MacBook Pro default
const Surface &trackpadSurface();

//! The touch frame of an IOHID digitizer event, or nothing for other events
std::optional<TouchFrame> touchFrame(IOHIDEventRef event, double time, const Surface &surface);

//! The touch frame carried by a Quartz gesture event, or nothing
std::optional<TouchFrame> touchFrame(CGEventRef event);

//! Zoom steps are the units of NX_ZOOM events: a step of +n multiplies the
//! zoom factor by 1 + n/100, a step of -n by 1 / (1 + n/100).
inline constexpr double kStepScale = 100.0;

//! Recognizes the Zoom trackpad gesture from touch frames
/*!
Same thresholds as the macOS recognizer: taps last at most 0.35 s and move
at most 15 mm, the second tap starts within 0.25 s of the first lift, and a
drag of 1 mm changes the zoom by 5 %. Up zooms in.
*/
class Recognizer
{
public:
  struct Result
  {
    bool toggle = false;     //!< a three-finger double-tap ended
    bool dragBegan = false;  //!< a zoom drag started (before its first steps)
    int steps = 0;           //!< zoom steps to apply now
    bool dragEnded = false;  //!< the zoom drag is over
  };

  Result update(const TouchFrame &frame);

  //! True while the second touch of a double-tap is down. It belongs to
  //! Zoom, so its Dock swipes and pinches must not reach the secondary.
  bool holdsGestures() const;

  void reset();

private:
  enum class State
  {
    Idle,
    FirstTouch,
    BetweenTaps,
    SecondTouch,
    Zooming,
    Ignored
  };

  void beginTouch(State state, const TouchFrame &frame);
  void landed(const TouchFrame &frame);
  bool isTap(const TouchFrame &lift) const;

  State m_state = State::Idle;
  bool m_holds = false;
  double m_lastTime = -1;
  double m_touchBegan = 0;
  double m_lifted = 0;
  bool m_landed = false;
  bool m_tooManyFingers = false;
  double m_landX = 0;
  double m_landY = 0;
  double m_maxDeviation = 0;
  double m_previousY = 0;
  double m_pendingSteps = 0;
};

//! A Zoom command sent to the secondary
struct Command
{
  enum class Kind
  {
    Toggle,
    Step,
    DragBegin, //!< show the zoom-in pointer, like macOS during the drag
    DragEnd    //!< restore the pointer
  };

  Kind kind = Kind::Toggle;
  int steps = 0;
};

//! Zoom commands travel in kMsgDTrackpadEvent, next to serialized gesture
//! events. They begin with "DFZM", serialized events with 00 00 00 02; an
//! older secondary cannot read them as an event and ignores them.
std::string encode(const Command &command);
std::optional<Command> decode(const std::string &bytes);

//! NX_ZOOM steps that take the zoom factor from @p from to @p to in
//! @p frames even parts (used to animate a toggle like macOS does)
std::vector<int> stepsBetween(double from, double to, int frames);

//! Secondary: run a Zoom command through the native Zoom engine
void perform(const Command &command);

//! Secondary: end a zoom drag cut short (the cursor left this computer)
void cancelDrag();

//! The pointer macOS shows during a zoom drag (CoreCursor types)
enum class DragCursor
{
  None = 0,     //!< the arrow, after the drag
  ZoomIn = 42,  //!< magnifying glass with +
  ZoomOut = 43, //!< magnifying glass with -
};

//! Picks the drag pointer like the Zoom agent: zoom in at the start, then
//! switch whenever the factor moves 0.05 away from where it last switched
class DragCursorTracker
{
public:
  //! @return the pointer to show now
  DragCursor begin(double factor);
  //! @return a pointer to switch to, or nothing to keep the current one
  std::optional<DragCursor> step(int steps);

  double factor() const
  {
    return m_factor;
  }

private:
  double m_factor = 1;
  double m_reference = 1;
};

} // namespace deskflow::osx::zoom

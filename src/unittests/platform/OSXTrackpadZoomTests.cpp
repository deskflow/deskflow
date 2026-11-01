/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Lucas Magotteaux
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "OSXTrackpadZoomTests.h"

#include "platform/OSXTrackpadZoom.h"

#include <cmath>

using namespace deskflow::osx::zoom;

extern "C"
{
  IOHIDEventRef IOHIDEventCreateDigitizerEvent(
      CFAllocatorRef allocator, uint64_t timeStamp, uint32_t transducerType, uint32_t index, uint32_t identity,
      uint32_t eventMask, uint32_t buttonMask, double x, double y, double z, double tipPressure, double barrelPressure,
      Boolean range, Boolean touch, uint32_t options
  );
  IOHIDEventRef IOHIDEventCreateDigitizerFingerEvent(
      CFAllocatorRef allocator, uint64_t timeStamp, uint32_t index, uint32_t identity, uint32_t eventMask, double x,
      double y, double z, double tipPressure, double twist, Boolean range, Boolean touch, uint32_t options
  );
  IOHIDEventRef IOHIDEventCreateKeyboardEvent(
      CFAllocatorRef allocator, uint64_t timeStamp, uint32_t usagePage, uint32_t usage, Boolean down, uint32_t flags
  );
  void IOHIDEventAppendEvent(IOHIDEventRef parent, IOHIDEventRef child, uint32_t options);
}

namespace {

constexpr uint32_t kTransducerHand = 3;

//! Feeds touch frames ~100 Hz, like a trackpad
struct Trackpad
{
  Recognizer recognizer;
  double time = 1000;
  int toggles = 0;
  int steps = 0;
  int dragsBegan = 0;
  int dragsEnded = 0;
  bool repeatEachSample = false;

  void frame(int fingers, double y = 40, double x = 60)
  {
    time += 0.01;
    const TouchFrame sample{time, fingers, x, y};
    for (int i = 0; i < (repeatEachSample ? 2 : 1); ++i) {
      const auto result = recognizer.update(sample);
      toggles += result.toggle ? 1 : 0;
      // the pointer changes before the first step and after the last one
      QVERIFY(!result.dragBegan || dragsBegan == dragsEnded);
      QVERIFY(result.steps == 0 || dragsBegan + (result.dragBegan ? 1 : 0) > dragsEnded);
      dragsBegan += result.dragBegan ? 1 : 0;
      steps += result.steps;
      dragsEnded += result.dragEnded ? 1 : 0;
    }
  }

  void hold(int fingers, double seconds, double y = 40)
  {
    for (double elapsed = 0; elapsed < seconds; elapsed += 0.01) {
      frame(fingers, y);
    }
  }

  //! no frames arrive while no finger touches
  void pause(double seconds)
  {
    time += seconds;
  }

  void land(int fingers = 3, double y = 40)
  {
    for (int count = 1; count <= fingers; ++count) {
      frame(count, y);
    }
  }

  void lift(int fingers = 3, double y = 40)
  {
    for (int count = fingers - 1; count >= 0; --count) {
      frame(count, y);
    }
  }

  void tap(double down = 0.1, int fingers = 3, double travel = 0)
  {
    land(fingers);
    const int samples = static_cast<int>(down / 0.01);
    for (int i = 0; i < samples; ++i) {
      frame(fingers, 40 + travel * i / std::max(1, samples - 1));
    }
    lift(fingers, 40 + travel);
  }

  void drag(double millimetres, int samples = 20)
  {
    land();
    for (int i = 1; i <= samples; ++i) {
      frame(3, 40 + millimetres * i / samples);
    }
    lift(3, 40 + millimetres);
  }
};

double applySteps(double factor, const std::vector<int> &steps)
{
  for (int step : steps) {
    factor *= step >= 0 ? 1 + step / kStepScale : 1 / (1 - step / kStepScale);
  }
  return factor;
}

} // namespace

void OSXTrackpadZoomTests::touchFrameCountsTouchingFingers()
{
  IOHIDEventRef hand = IOHIDEventCreateDigitizerEvent(nullptr, 1, kTransducerHand, 0, 0, 0, 0, 0.5, 0.5, 0, 0, 0, true, true, 0);
  const double positions[] = {0.2, 0.4, 0.6};
  for (uint32_t i = 0; i < 3; ++i) {
    IOHIDEventRef finger = IOHIDEventCreateDigitizerFingerEvent(
        nullptr, 1, i, i, 0, positions[i], positions[i], 0, 0.5, 0, true, true, 0
    );
    IOHIDEventAppendEvent(hand, finger, 0);
    CFRelease(finger);
  }
  // in range but not touching: a hovering or lifting finger
  IOHIDEventRef hovering = IOHIDEventCreateDigitizerFingerEvent(nullptr, 1, 3, 3, 0, 0.9, 0.9, 0, 0, 0, true, false, 0);
  IOHIDEventAppendEvent(hand, hovering, 0);
  CFRelease(hovering);

  const auto frame = touchFrame(hand, 12.5, Surface{100, 50});
  CFRelease(hand);

  QVERIFY(frame.has_value());
  QCOMPARE(frame->time, 12.5);
  QCOMPARE(frame->fingers, 3);
  QCOMPARE(frame->x, 40.0);
  // digitizer y grows downward; frames measure from the bottom edge
  QCOMPARE(frame->y, 30.0);
}

void OSXTrackpadZoomTests::touchFrameIgnoresOtherEvents()
{
  QVERIFY(!touchFrame(nullptr, 0, Surface{}).has_value());

  IOHIDEventRef key = IOHIDEventCreateKeyboardEvent(nullptr, 1, 7, 4, true, 0);
  QVERIFY(!touchFrame(key, 0, Surface{}).has_value());
  CFRelease(key);

  // all fingers lifted is still a frame
  IOHIDEventRef hand = IOHIDEventCreateDigitizerEvent(nullptr, 1, kTransducerHand, 0, 0, 0, 0, 0, 0, 0, 0, 0, false, false, 0);
  const auto lifted = touchFrame(hand, 0, Surface{});
  CFRelease(hand);
  QVERIFY(lifted.has_value());
  QCOMPARE(lifted->fingers, 0);
}

void OSXTrackpadZoomTests::doubleTapToggles()
{
  Trackpad trackpad;
  trackpad.tap();
  trackpad.pause(0.1);
  trackpad.tap();

  QCOMPARE(trackpad.toggles, 1);
  QCOMPARE(trackpad.steps, 0);
  QVERIFY(!trackpad.recognizer.holdsGestures());

  // a third tap starts over rather than toggling again
  trackpad.pause(0.1);
  trackpad.tap();
  QCOMPARE(trackpad.toggles, 1);
}

void OSXTrackpadZoomTests::doubleTapAndDragZooms_data()
{
  QTest::addColumn<double>("millimetres");
  QTest::newRow("up zooms in") << 10.0;
  QTest::newRow("down zooms out") << -10.0;
}

void OSXTrackpadZoomTests::doubleTapAndDragZooms()
{
  QFETCH(double, millimetres);

  Trackpad trackpad;
  trackpad.tap();
  trackpad.pause(0.1);
  trackpad.drag(millimetres);

  QCOMPARE(trackpad.toggles, 0);
  // 5 steps per mm past the 1.5 mm needed to tell a drag from a tap
  const double expected = (std::abs(millimetres) - 1.5) * 5;
  QVERIFY2(std::abs(std::abs(trackpad.steps) - expected) <= 5, qPrintable(QString::number(trackpad.steps)));
  QCOMPARE(trackpad.steps > 0, millimetres > 0);
}

void OSXTrackpadZoomTests::dragHoldsGesturesUntilLift()
{
  Trackpad trackpad;
  trackpad.land();
  QVERIFY(!trackpad.recognizer.holdsGestures());
  trackpad.hold(3, 0.1);
  trackpad.lift();
  QVERIFY(!trackpad.recognizer.holdsGestures());

  trackpad.pause(0.1);
  trackpad.land();
  QVERIFY(trackpad.recognizer.holdsGestures());
  // holding still past the tap duration is still the Zoom gesture
  trackpad.hold(3, 1.0);
  QVERIFY(trackpad.recognizer.holdsGestures());
  for (int i = 1; i <= 10; ++i) {
    trackpad.frame(3, 40 + i);
  }
  QVERIFY(trackpad.recognizer.holdsGestures());
  QVERIFY(trackpad.steps > 0);
  trackpad.lift(3, 50);
  QVERIFY(!trackpad.recognizer.holdsGestures());
  QCOMPARE(trackpad.toggles, 0);

  // lifting a finger ends the drag; the others moving do not zoom
  trackpad.pause(1);
  trackpad.tap();
  trackpad.pause(0.1);
  trackpad.land();
  for (int i = 1; i <= 5; ++i) {
    trackpad.frame(3, 40 + i);
  }
  const int stepsBeforeLift = trackpad.steps;
  trackpad.hold(2, 0.1, 30);
  trackpad.hold(2, 0.1, 10);
  QCOMPARE(trackpad.steps, stepsBeforeLift);
  QVERIFY(trackpad.recognizer.holdsGestures());
  trackpad.frame(0);
  QVERIFY(!trackpad.recognizer.holdsGestures());
}

void OSXTrackpadZoomTests::dragBeginsAndEndsOnce_data()
{
  QTest::addColumn<int>("fingersAfterDrag");
  QTest::newRow("all fingers lift") << 0;
  QTest::newRow("one finger lifts first") << 2;
  QTest::newRow("a fourth finger lands") << 4;
}

void OSXTrackpadZoomTests::dragBeginsAndEndsOnce()
{
  QFETCH(int, fingersAfterDrag);

  Trackpad trackpad;
  trackpad.tap();
  trackpad.pause(0.1);
  trackpad.land();
  for (int i = 1; i <= 10; ++i) {
    trackpad.frame(3, 40 + i);
  }
  QCOMPARE(trackpad.dragsBegan, 1);
  QCOMPARE(trackpad.dragsEnded, 0);

  trackpad.hold(fingersAfterDrag, 0.05, 50);
  trackpad.frame(0);
  QCOMPARE(trackpad.dragsBegan, 1);
  QCOMPARE(trackpad.dragsEnded, 1);

  // a double-tap does not drag
  trackpad.pause(1);
  trackpad.tap();
  trackpad.pause(0.1);
  trackpad.tap();
  QCOMPARE(trackpad.toggles, 1);
  QCOMPARE(trackpad.dragsBegan, 1);
  QCOMPARE(trackpad.dragsEnded, 1);
}

void OSXTrackpadZoomTests::notADoubleTap_data()
{
  QTest::addColumn<int>("scenario");
  QTest::newRow("single tap") << 0;
  QTest::newRow("second tap too late") << 1;
  QTest::newRow("first tap too long") << 2;
  QTest::newRow("second tap too long") << 3;
  QTest::newRow("four fingers") << 4;
  QTest::newRow("two fingers") << 5;
  QTest::newRow("first tap moves") << 6;
}

void OSXTrackpadZoomTests::notADoubleTap()
{
  QFETCH(int, scenario);

  Trackpad trackpad;
  switch (scenario) {
  case 0:
    trackpad.tap();
    break;
  case 1:
    trackpad.tap();
    trackpad.pause(0.4);
    trackpad.tap();
    break;
  case 2:
    trackpad.tap(0.5);
    trackpad.pause(0.1);
    trackpad.tap();
    break;
  case 3:
    trackpad.tap();
    trackpad.pause(0.1);
    trackpad.tap(0.5);
    break;
  case 4:
    trackpad.tap(0.1, 4);
    trackpad.pause(0.1);
    trackpad.tap(0.1, 4);
    break;
  case 5:
    trackpad.tap(0.1, 2);
    trackpad.pause(0.1);
    trackpad.tap(0.1, 2);
    break;
  case 6:
    trackpad.tap(0.2, 3, 20);
    trackpad.pause(0.1);
    trackpad.tap();
    break;
  default:
    QFAIL("unknown scenario");
  }

  QCOMPARE(trackpad.toggles, 0);
  QCOMPARE(trackpad.steps, 0);
  QVERIFY(!trackpad.recognizer.holdsGestures());
}

void OSXTrackpadZoomTests::staggeredLandingAndLift()
{
  Trackpad trackpad;
  trackpad.tap();
  trackpad.pause(0.1);
  // the centroid jumps as fingers land and lift one by one: not a drag
  trackpad.frame(1, 10);
  trackpad.frame(2, 25);
  trackpad.hold(3, 0.1);
  trackpad.frame(2, 60);
  trackpad.frame(1, 5);
  trackpad.frame(0);

  QCOMPARE(trackpad.toggles, 1);
  QCOMPARE(trackpad.steps, 0);
}

void OSXTrackpadZoomTests::repeatedSamplesAreIgnored()
{
  Trackpad once;
  once.tap();
  once.pause(0.1);
  once.drag(10);

  Trackpad twice;
  twice.repeatEachSample = true;
  twice.tap();
  twice.pause(0.1);
  twice.drag(10);

  QCOMPARE(twice.steps, once.steps);
  QCOMPARE(twice.toggles, 0);

  twice.pause(1);
  twice.tap();
  twice.pause(0.1);
  twice.tap();
  QCOMPARE(twice.toggles, 1);
}

void OSXTrackpadZoomTests::commandRoundTrip()
{
  const auto toggle = decode(encode({Command::Kind::Toggle, 0}));
  QVERIFY(toggle.has_value());
  QVERIFY(toggle->kind == Command::Kind::Toggle);

  for (const auto kind : {Command::Kind::DragBegin, Command::Kind::DragEnd}) {
    const auto command = decode(encode({kind, 0}));
    QVERIFY(command.has_value());
    QVERIFY(command->kind == kind);
  }

  const std::pair<int, int> steps[] = {{1, 1}, {-1, -1}, {300, 300}, {-32767, -32767}, {40000, 32767}, {-40000, -32767}};
  for (const auto &[sent, received] : steps) {
    const auto step = decode(encode({Command::Kind::Step, sent}));
    QVERIFY(step.has_value());
    QVERIFY(step->kind == Command::Kind::Step);
    QCOMPARE(step->steps, received);
  }
}

void OSXTrackpadZoomTests::serializedEventsAreNotCommands()
{
  CGEventRef event = CGEventCreate(nullptr);
  CGEventSetType(event, static_cast<CGEventType>(29));
  CFDataRef data = CGEventCreateData(kCFAllocatorDefault, event);
  const std::string bytes(reinterpret_cast<const char *>(CFDataGetBytePtr(data)), CFDataGetLength(data));
  CFRelease(data);
  CFRelease(event);

  QVERIFY(!decode(bytes).has_value());
  QVERIFY(!decode("").has_value());
  QVERIFY(!decode("DFZM").has_value());
  QVERIFY(!decode("DFZMX").has_value());
  QVERIFY(!decode(std::string("DFZMS\x01", 6)).has_value());
  QVERIFY(!decode("DFZMTT").has_value());
}

void OSXTrackpadZoomTests::toggleStepsReachTarget_data()
{
  QTest::addColumn<double>("from");
  QTest::addColumn<double>("to");
  QTest::newRow("in to 2x") << 1.0 << 2.0;
  QTest::newRow("in to 40x") << 1.0 << 40.0;
  QTest::newRow("in to 1.05x") << 1.0 << 1.05;
  QTest::newRow("out from 5x") << 5.0 << 1.0;
  QTest::newRow("out from 1.21x") << 1.21 << 1.0;
}

void OSXTrackpadZoomTests::toggleStepsReachTarget()
{
  QFETCH(double, from);
  QFETCH(double, to);

  const auto steps = stepsBetween(from, to, 8);
  QVERIFY(!steps.empty());
  QVERIFY(steps.size() <= 8);
  for (int step : steps) {
    QCOMPARE(step > 0, to > from);
  }
  const double reached = applySteps(from, steps);
  QVERIFY2(std::abs(reached - to) / to < 0.01, qPrintable(QString::number(reached)));
}

void OSXTrackpadZoomTests::dragCursorFollowsDirection()
{
  DragCursorTracker tracker;
  QVERIFY(tracker.begin(1) == DragCursor::ZoomIn);

  // small moves keep the pointer, like Zoom's 0.05 hysteresis
  QVERIFY(!tracker.step(3).has_value());
  QVERIFY(!tracker.step(-3).has_value());

  // zooming in further keeps the zoom-in pointer
  const auto in = tracker.step(10);
  QVERIFY(in.has_value() && *in == DragCursor::ZoomIn);

  // turning back switches to zoom out once past the hysteresis
  QVERIFY(!tracker.step(-4).has_value());
  const auto out = tracker.step(-4);
  QVERIFY(out.has_value() && *out == DragCursor::ZoomOut);

  // and back again
  QVERIFY(!tracker.step(2).has_value());
  const auto backIn = tracker.step(5);
  QVERIFY(backIn.has_value() && *backIn == DragCursor::ZoomIn);
}

void OSXTrackpadZoomTests::dragCursorStaysWithinZoomLimits()
{
  DragCursorTracker tracker;
  tracker.begin(1);
  // at 1x Zoom cannot zoom out: the factor and the pointer stay
  for (int i = 0; i < 10; ++i) {
    QVERIFY(!tracker.step(-20).has_value());
  }
  QCOMPARE(tracker.factor(), 1.0);

  tracker.begin(40);
  for (int i = 0; i < 10; ++i) {
    QVERIFY(!tracker.step(20).has_value());
  }
  QCOMPARE(tracker.factor(), 40.0);
  const auto out = tracker.step(-20);
  QVERIFY(out.has_value() && *out == DragCursor::ZoomOut);
}

QTEST_MAIN(OSXTrackpadZoomTests)

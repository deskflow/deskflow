/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Lucas Magotteaux
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/OSXTrackpadZoom.h"

#include "base/Log.h"

#include <IOKit/IOKitLib.h>
#include <dispatch/dispatch.h>
#include <dlfcn.h>
#include <mach/mach_time.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <mutex>

// Private but long-standing IOKit and Quartz entry points (exported, not in
// the SDK headers). Weak, so a missing one disables the feature.
extern "C"
{
  IOHIDEventRef CGEventCopyIOHIDEvent(CGEventRef event) __attribute__((weak_import));
  CFArrayRef IOHIDEventGetChildren(IOHIDEventRef event) __attribute__((weak_import));
  uint32_t IOHIDEventGetType(IOHIDEventRef event) __attribute__((weak_import));
  double IOHIDEventGetFloatValue(IOHIDEventRef event, uint32_t field) __attribute__((weak_import));
  CFIndex IOHIDEventGetIntegerValue(IOHIDEventRef event, uint32_t field) __attribute__((weak_import));
  uint64_t IOHIDEventGetTimeStamp(IOHIDEventRef event) __attribute__((weak_import));
  // HIServices; what AXFMouse uses for the Zoom drag pointer
  CGError CoreCursorSetAndReturnSeed(int connection, int cursor, int *seed) __attribute__((weak_import));
  int _CGSDefaultConnection();
}

namespace deskflow::osx::zoom {
namespace {

// IOHIDEventTypes.h / IOHIDEventFieldDefs.h
constexpr uint32_t kHIDEventTypeDigitizer = 11;
constexpr uint32_t kFieldDigitizerX = kHIDEventTypeDigitizer << 16;
constexpr uint32_t kFieldDigitizerY = kFieldDigitizerX + 1;
constexpr uint32_t kFieldDigitizerType = kFieldDigitizerX + 4;
constexpr uint32_t kFieldDigitizerTouch = kFieldDigitizerX + 9;
constexpr CFIndex kTransducerHand = 3;

// NX_ZOOM, IOLLEvent.h. AXVisualSupportAgent reads its scroll delta (axis 1)
// as zoom steps; momentum phase 0 applies the step to the cursor's screen.
constexpr auto kEventTypeZoom = static_cast<CGEventType>(28);

// the macOS recognizer (AXVisualSupportAgent, AccessibilitySupport defaults)
constexpr int kFingers = 3;
constexpr double kMaxTapDuration = 0.35;
constexpr double kMaxTapInterval = 0.25;
constexpr double kMaxTapDeviation = 15.0;           // mm
constexpr double kStepsPerMillimetre = 0.05 * kStepScale;
// vertical motion before the second touch counts as a drag, not a tap: the
// centroid jitters by a fraction of a millimetre while fingers rest
constexpr double kDragThreshold = 1.5; // mm

constexpr int kMaxStep = 32767; // NX_ZOOM steps are 16 bit

constexpr char kMagic[] = {'D', 'F', 'Z', 'M'};
constexpr char kKindToggle = 'T';
constexpr char kKindStep = 'S';
constexpr char kKindDragBegin = 'B';
constexpr char kKindDragEnd = 'E';

// the Zoom agent switches the drag pointer when the factor moved this much
constexpr double kCursorHysteresis = 0.05;
constexpr double kMinFactor = 1;
constexpr double kMaxFactor = 40;

double seconds(uint64_t machTime)
{
  static const mach_timebase_info_data_t timebase = [] {
    mach_timebase_info_data_t info{};
    mach_timebase_info(&info);
    return info;
  }();
  return static_cast<double>(machTime) * timebase.numer / timebase.denom / 1e9;
}

bool hidAvailable()
{
  return CGEventCopyIOHIDEvent != nullptr && IOHIDEventGetChildren != nullptr && IOHIDEventGetType != nullptr &&
         IOHIDEventGetFloatValue != nullptr && IOHIDEventGetIntegerValue != nullptr &&
         IOHIDEventGetTimeStamp != nullptr;
}

struct FingerSum
{
  bool digitizer = false;
  int fingers = 0;
  double x = 0;
  double y = 0;
};

void addFingers(IOHIDEventRef event, FingerSum &sum, int depth)
{
  bool hasDigitizerChild = false;
  if (CFArrayRef children = depth < 4 ? IOHIDEventGetChildren(event) : nullptr; children != nullptr) {
    for (CFIndex i = 0; i < CFArrayGetCount(children); ++i) {
      auto child = static_cast<IOHIDEventRef>(const_cast<void *>(CFArrayGetValueAtIndex(children, i)));
      hasDigitizerChild |= IOHIDEventGetType(child) == kHIDEventTypeDigitizer;
      addFingers(child, sum, depth + 1);
    }
  }
  if (IOHIDEventGetType(event) != kHIDEventTypeDigitizer) {
    return;
  }
  sum.digitizer = true;
  // a finger is a touching digitizer leaf; its parent collection (the hand)
  // reports touch as long as any finger does
  if (!hasDigitizerChild && IOHIDEventGetIntegerValue(event, kFieldDigitizerType) != kTransducerHand &&
      IOHIDEventGetIntegerValue(event, kFieldDigitizerTouch) != 0) {
    ++sum.fingers;
    sum.x += IOHIDEventGetFloatValue(event, kFieldDigitizerX);
    sum.y += IOHIDEventGetFloatValue(event, kFieldDigitizerY);
  }
}

double registryNumber(io_object_t service, CFStringRef key)
{
  double value = 0;
  if (CFTypeRef property = IORegistryEntryCreateCFProperty(service, key, kCFAllocatorDefault, 0); property != nullptr) {
    if (CFGetTypeID(property) == CFNumberGetTypeID()) {
      CFNumberGetValue(static_cast<CFNumberRef>(property), kCFNumberDoubleType, &value);
    }
    CFRelease(property);
  }
  return value;
}

double factorOf(int step)
{
  return step >= 0 ? 1 + step / kStepScale : 1 / (1 - step / kStepScale);
}

int clampStep(long step)
{
  return static_cast<int>(std::clamp<long>(step, -kMaxStep, kMaxStep));
}

//! Zoom state and settings, from the private UniversalAccessCore framework
class UniversalAccess
{
public:
  static const UniversalAccess &get()
  {
    static const UniversalAccess instance;
    return instance;
  }

  bool loaded() const
  {
    return trackpadZoomEnabled != nullptr && scrollZoomEnabled != nullptr && zoomedIn != nullptr &&
           scale != nullptr && desiredScale != nullptr;
  }

  bool (*trackpadZoomEnabled)() = nullptr;
  bool (*scrollZoomEnabled)() = nullptr;
  bool (*zoomedIn)(CGPoint) = nullptr;
  double (*scale)() = nullptr;
  double (*desiredScale)() = nullptr;

private:
  UniversalAccess()
  {
    void *library = dlopen(
        "/System/Library/PrivateFrameworks/UniversalAccess.framework/Versions/A/Frameworks/"
        "UniversalAccessCore.framework/Versions/A/UniversalAccessCore",
        RTLD_NOW | RTLD_LOCAL
    );
    if (library == nullptr) {
      return;
    }
    trackpadZoomEnabled = reinterpret_cast<bool (*)()>(dlsym(library, "UATrackpadGestureZoomIsEnabled"));
    scrollZoomEnabled = reinterpret_cast<bool (*)()>(dlsym(library, "UAScrollZoomIsEnabled"));
    zoomedIn = reinterpret_cast<bool (*)(CGPoint)>(dlsym(library, "UAZoomIsLocationZoomedIn"));
    scale = reinterpret_cast<double (*)()>(dlsym(library, "UAZoomScale"));
    desiredScale = reinterpret_cast<double (*)()>(dlsym(library, "UAZoomDesiredScale"));
  }
};

void postZoomEvent(int step)
{
  CGEventRef event = CGEventCreate(nullptr); // at the cursor
  if (event == nullptr) {
    return;
  }
  CGEventSetType(event, kEventTypeZoom);
  CGEventSetIntegerValueField(event, kCGScrollWheelEventDeltaAxis1, step);
  CGEventPost(kCGSessionEventTap, event);
  CFRelease(event);
}

//! Spread toggle steps over a few frames, as the macOS toggle animates
void postAnimated(const std::vector<int> &steps)
{
  constexpr int64_t kFrameInterval = 16 * NSEC_PER_MSEC;
  static dispatch_queue_t queue = dispatch_queue_create("org.deskflow.zoom", DISPATCH_QUEUE_SERIAL);
  for (size_t i = 0; i < steps.size(); ++i) {
    const int step = steps[i];
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, static_cast<int64_t>(i) * kFrameInterval), queue, ^{
      postZoomEvent(step);
    });
  }
}

} // namespace

const Surface &trackpadSurface()
{
  static const Surface surface = [] {
    Surface result;
    io_iterator_t services = IO_OBJECT_NULL;
    // MACH_PORT_NULL is the default main port
    if (IOServiceGetMatchingServices(MACH_PORT_NULL, IOServiceMatching("AppleMultitouchDevice"), &services) !=
        KERN_SUCCESS) {
      return result;
    }
    while (io_object_t service = IOIteratorNext(services)) {
      // hundredths of a millimetre
      const double width = registryNumber(service, CFSTR("Sensor Surface Width"));
      const double height = registryNumber(service, CFSTR("Sensor Surface Height"));
      IOObjectRelease(service);
      if (width > 0 && height > 0) {
        result = {width / 100, height / 100};
        break;
      }
    }
    IOObjectRelease(services);
    return result;
  }();
  return surface;
}

std::optional<TouchFrame> touchFrame(IOHIDEventRef event, double time, const Surface &surface)
{
  if (event == nullptr || !hidAvailable()) {
    return std::nullopt;
  }
  FingerSum sum;
  addFingers(event, sum, 0);
  if (!sum.digitizer) {
    return std::nullopt;
  }
  TouchFrame frame;
  frame.time = time;
  frame.fingers = sum.fingers;
  if (sum.fingers > 0) {
    // digitizer coordinates are normalized, y growing downward
    frame.x = sum.x / sum.fingers * surface.width;
    frame.y = (1 - sum.y / sum.fingers) * surface.height;
  }
  return frame;
}

std::optional<TouchFrame> touchFrame(CGEventRef event)
{
  if (!hidAvailable()) {
    return std::nullopt;
  }
  IOHIDEventRef hid = CGEventCopyIOHIDEvent(event);
  if (hid == nullptr) {
    return std::nullopt;
  }
  const uint64_t timestamp = IOHIDEventGetTimeStamp(hid);
  auto frame = touchFrame(hid, seconds(timestamp != 0 ? timestamp : mach_absolute_time()), trackpadSurface());
  CFRelease(hid);
  return frame;
}

Recognizer::Result Recognizer::update(const TouchFrame &frame)
{
  // one sample can arrive in several gesture events
  if (frame.time == m_lastTime) {
    return {};
  }
  m_lastTime = frame.time;

  Result result;
  switch (m_state) {
  case State::Idle:
    if (frame.fingers > 0) {
      beginTouch(State::FirstTouch, frame);
    }
    break;

  case State::FirstTouch:
    landed(frame);
    if (frame.fingers == 0) {
      m_state = isTap(frame) ? State::BetweenTaps : State::Idle;
      m_lifted = frame.time;
    } else if (m_tooManyFingers || frame.time - m_touchBegan > kMaxTapDuration) {
      m_state = State::Ignored;
    }
    break;

  case State::BetweenTaps:
    if (frame.fingers > 0) {
      beginTouch(frame.time - m_lifted <= kMaxTapInterval ? State::SecondTouch : State::FirstTouch, frame);
    }
    break;

  case State::SecondTouch:
    landed(frame);
    if (frame.fingers == 0) {
      result.toggle = isTap(frame);
      reset();
    } else if (m_tooManyFingers) {
      m_state = State::Ignored;
    } else if (m_landed && frame.fingers == kFingers && std::abs(frame.y - m_landY) >= kDragThreshold) {
      m_state = State::Zooming;
      m_previousY = frame.y;
      m_pendingSteps = 0;
      result.dragBegan = true;
    }
    break;

  case State::Zooming:
    if (frame.fingers == kFingers) {
      m_pendingSteps += (frame.y - m_previousY) * kStepsPerMillimetre;
      m_previousY = frame.y;
      result.steps = static_cast<int>(m_pendingSteps);
      m_pendingSteps -= result.steps;
    } else {
      result.dragEnded = true;
      if (frame.fingers == 0) {
        reset();
      } else {
        // a finger lifted: the drag is over, wait for the others
        m_state = State::Ignored;
      }
    }
    break;

  case State::Ignored:
    if (frame.fingers == 0) {
      reset();
    }
    break;
  }
  return result;
}

bool Recognizer::holdsGestures() const
{
  return m_holds;
}

void Recognizer::reset()
{
  m_state = State::Idle;
  m_holds = false;
}

void Recognizer::beginTouch(State state, const TouchFrame &frame)
{
  m_state = state;
  m_holds = state == State::SecondTouch;
  m_touchBegan = frame.time;
  m_landed = false;
  m_tooManyFingers = false;
  m_maxDeviation = 0;
  landed(frame);
}

void Recognizer::landed(const TouchFrame &frame)
{
  if (frame.fingers > kFingers) {
    m_tooManyFingers = true;
  }
  if (frame.fingers != kFingers) {
    return;
  }
  // fingers land one after the other; measure from the first full frame
  if (!m_landed) {
    m_landed = true;
    m_landX = frame.x;
    m_landY = frame.y;
  } else {
    m_maxDeviation = std::max(m_maxDeviation, std::hypot(frame.x - m_landX, frame.y - m_landY));
  }
}

bool Recognizer::isTap(const TouchFrame &lift) const
{
  return m_landed && !m_tooManyFingers && lift.time - m_touchBegan <= kMaxTapDuration &&
         m_maxDeviation <= kMaxTapDeviation;
}

std::string encode(const Command &command)
{
  std::string bytes(kMagic, sizeof(kMagic));
  switch (command.kind) {
  case Command::Kind::Toggle:
    bytes += kKindToggle;
    break;
  case Command::Kind::DragBegin:
    bytes += kKindDragBegin;
    break;
  case Command::Kind::DragEnd:
    bytes += kKindDragEnd;
    break;
  case Command::Kind::Step: {
    const auto step = static_cast<uint16_t>(static_cast<int16_t>(clampStep(command.steps)));
    bytes += kKindStep;
    bytes += static_cast<char>(step >> 8);
    bytes += static_cast<char>(step & 0xff);
    break;
  }
  }
  return bytes;
}

std::optional<Command> decode(const std::string &bytes)
{
  if (bytes.size() < sizeof(kMagic) + 1 || bytes.compare(0, sizeof(kMagic), kMagic, sizeof(kMagic)) != 0) {
    return std::nullopt;
  }
  const char kind = bytes[sizeof(kMagic)];
  if (bytes.size() == sizeof(kMagic) + 1) {
    switch (kind) {
    case kKindToggle:
      return Command{Command::Kind::Toggle, 0};
    case kKindDragBegin:
      return Command{Command::Kind::DragBegin, 0};
    case kKindDragEnd:
      return Command{Command::Kind::DragEnd, 0};
    default:
      return std::nullopt;
    }
  }
  if (kind == kKindStep && bytes.size() == sizeof(kMagic) + 3) {
    const auto high = static_cast<uint8_t>(bytes[sizeof(kMagic) + 1]);
    const auto low = static_cast<uint8_t>(bytes[sizeof(kMagic) + 2]);
    return Command{Command::Kind::Step, static_cast<int16_t>(static_cast<uint16_t>(high << 8 | low))};
  }
  return std::nullopt;
}

std::vector<int> stepsBetween(double from, double to, int frames)
{
  std::vector<int> steps;
  if (from <= 0 || to <= 0 || frames <= 0) {
    return steps;
  }
  double applied = from;
  for (int i = 1; i <= frames; ++i) {
    const double ratio = from * std::pow(to / from, static_cast<double>(i) / frames) / applied;
    const long step = ratio >= 1 ? std::lround((ratio - 1) * kStepScale) : -std::lround((1 / ratio - 1) * kStepScale);
    if (step != 0) {
      steps.push_back(clampStep(step));
      applied *= factorOf(steps.back());
    }
  }
  return steps;
}

DragCursor DragCursorTracker::begin(double factor)
{
  m_factor = std::clamp(factor, kMinFactor, kMaxFactor);
  m_reference = m_factor;
  return DragCursor::ZoomIn;
}

std::optional<DragCursor> DragCursorTracker::step(int steps)
{
  m_factor = std::clamp(m_factor * factorOf(steps), kMinFactor, kMaxFactor);
  if (m_factor < m_reference - kCursorHysteresis || m_factor > m_reference + kCursorHysteresis) {
    const auto cursor = m_factor < m_reference ? DragCursor::ZoomOut : DragCursor::ZoomIn;
    m_reference = m_factor;
    return cursor;
  }
  return std::nullopt;
}

namespace {

void setCursor(DragCursor cursor)
{
  if (CoreCursorSetAndReturnSeed == nullptr) {
    return;
  }
  // shown although Deskflow is in the background: the secondary marks its
  // connection SetsCursorInBackground when the cursor enters (showCursor)
  int seed = 0;
  if (const auto error = CoreCursorSetAndReturnSeed(_CGSDefaultConnection(), static_cast<int>(cursor), &seed);
      error != kCGErrorSuccess) {
    LOG_DEBUG("failed to set the zoom drag pointer, error=%d", error);
  }
}

CGPoint cursorLocation()
{
  CGPoint location = CGPointZero;
  if (CGEventRef now = CGEventCreate(nullptr); now != nullptr) {
    location = CGEventGetLocation(now);
    CFRelease(now);
  }
  return location;
}

//! Secondary state, used from the client event thread
struct Client
{
  std::mutex mutex;
  double restoreFactor = 0;
  std::optional<DragCursorTracker> drag;
  std::atomic<bool> warnedScrollZoom{false};

  static Client &get()
  {
    static Client client;
    return client;
  }

  void toggle(const UniversalAccess &ua)
  {
    constexpr int kToggleFrames = 8;
    // Zoom clamps the factor at 1x, so overshooting guarantees Zoom turns off
    // even if the saved factor lags behind the live one
    constexpr int kZoomOutRest = -1000;

    if (ua.zoomedIn(cursorLocation())) {
      // like the macOS toggle, remember the level to come back to
      const double current = std::clamp(ua.scale(), kMinFactor, kMaxFactor);
      if (current > 1) {
        restoreFactor = current;
      }
      auto steps = stepsBetween(current, 1, kToggleFrames);
      steps.push_back(kZoomOutRest);
      LOG_DEBUG("zoom toggle: out from %.2fx", current);
      postAnimated(steps);
    } else {
      double target = restoreFactor > 1 ? restoreFactor : ua.desiredScale();
      if (target <= 1) {
        target = 2;
      }
      target = std::min(target, kMaxFactor);
      LOG_DEBUG("zoom toggle: in to %.2fx", target);
      postAnimated(stepsBetween(1, target, kToggleFrames));
    }
  }

  void endDrag()
  {
    if (drag.has_value()) {
      drag.reset();
      setCursor(DragCursor::None);
    }
  }
};

} // namespace

void perform(const Command &command)
{
  auto &client = Client::get();
  const auto &ua = UniversalAccess::get();
  if (!ua.loaded()) {
    LOG_DEBUG("ignoring zoom command, Universal Access is unavailable");
    return;
  }
  if (!ua.trackpadZoomEnabled()) {
    LOG_DEBUG("ignoring zoom command, the Zoom trackpad gesture is off on this computer");
    return;
  }
  if (!ua.scrollZoomEnabled()) {
    if (!client.warnedScrollZoom.exchange(true)) {
      LOG_WARN(
          "the Zoom trackpad gesture from the server needs \"Use scroll gesture with modifier keys to zoom\" "
          "turned on on this computer (System Settings > Accessibility > Zoom)"
      );
    }
    return;
  }
  client.warnedScrollZoom = false;

  std::scoped_lock lock(client.mutex);
  switch (command.kind) {
  case Command::Kind::Toggle:
    client.toggle(ua);
    break;

  case Command::Kind::DragBegin: {
    // Zoom compares absolute factors, so start from the current one
    const double factor = ua.zoomedIn(cursorLocation()) ? ua.scale() : 1;
    setCursor(client.drag.emplace().begin(factor));
    break;
  }

  case Command::Kind::Step:
    postZoomEvent(clampStep(command.steps));
    if (client.drag.has_value()) {
      if (const auto cursor = client.drag->step(command.steps)) {
        setCursor(*cursor);
      }
    }
    break;

  case Command::Kind::DragEnd:
    client.endDrag();
    break;
  }
}

void cancelDrag()
{
  auto &client = Client::get();
  std::scoped_lock lock(client.mutex);
  client.endDrag();
}

} // namespace deskflow::osx::zoom

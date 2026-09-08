/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/OSXInputLock.h"
#include "platform/OSXInputLockHeldInputs.h"

#include "base/IEventQueue.h"
#include "base/Log.h"
#include "deskflow/SharedInputLock.h"
#include "deskflow/SharedInputLockEvent.h"

#include <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>
#include <bitset>
#include <pthread.h>
#include <set>

using State = deskflow::SharedInputLock;
using Phase = State::Phase;
using Reason = State::Reason;

namespace {

void onMainSync(dispatch_block_t block)
{
  if (pthread_main_np()) {
    block();
  } else {
    dispatch_sync(dispatch_get_main_queue(), block);
  }
}

bool isModifier(CGKeyCode key)
{
  return key >= kVK_RightCommand && key <= kVK_Function;
}

} // namespace

struct OSXInputLock::Impl
{
  Impl(IEventQueue *events, void *target, std::function<void(bool)> restore)
      : events(events),
        target(target),
        restoreState(std::move(restore))
  {
  }

  bool healthy() const
  {
    return enabled && tap && CFMachPortIsValid(tap) && CGEventTapIsEnabled(tap) && !IsSecureEventInputEnabled();
  }

  bool inputsReleased() const
  {
    return held.readyToResume();
  }

  void post(deskflow::EventTypes type)
  {
    events->addEvent(Event(type, target, new deskflow::SharedInputLockEvent(state.generation())));
  }

  void showStatus(NSString *text)
  {
    if (!panel) {
      panel = [[NSPanel alloc] initWithContentRect:NSMakeRect(0, 0, 370, 92)
                                         styleMask:NSWindowStyleMaskBorderless | NSWindowStyleMaskNonactivatingPanel
                                           backing:NSBackingStoreBuffered
                                             defer:NO];
      [panel setReleasedWhenClosed:NO];
      [panel setLevel:NSStatusWindowLevel];
      [panel setHidesOnDeactivate:NO];
      [panel setIgnoresMouseEvents:YES];
      [panel setCollectionBehavior:NSWindowCollectionBehaviorCanJoinAllSpaces |
                                   NSWindowCollectionBehaviorFullScreenAuxiliary];
      label = [NSTextField labelWithString:@""];
      [label setFrame:NSMakeRect(16, 12, 338, 68)];
      [label setFont:[NSFont systemFontOfSize:15]];
      [label setMaximumNumberOfLines:3];
      [[panel contentView] addSubview:label];
    }
    [label setStringValue:text];
    NSRect visible = [[NSScreen mainScreen] visibleFrame];
    [panel setFrameTopLeftPoint:NSMakePoint(NSMaxX(visible) - 386, NSMaxY(visible) - 16)];
    [panel orderFrontRegardless];
  }

  void updateStatus()
  {
    if (shownPhase == state.phase() && shownReason == state.reason() && shownSuppression == state.suppressesLocal() &&
        shownAwaitingRemote == state.awaitingRemoteRelease() && shownHistoryLost == held.historyLost()) {
      return;
    }
    shownPhase = state.phase();
    shownReason = state.reason();
    shownSuppression = state.suppressesLocal();
    shownAwaitingRemote = state.awaitingRemoteRelease();
    shownHistoryLost = held.historyLost();
    switch (state.phase()) {
    case Phase::Open:
      [panel orderOut:nil];
      LOG_NOTE("shared input unlocked");
      break;
    case Phase::Preparing:
      showStatus(@"Preparing input lock…\nReleasing keys and mouse buttons");
      LOG_NOTE("preparing shared input lock");
      break;
    case Phase::Locked:
      showStatus(@"Input locked\nHold Esc for 3 seconds to unlock");
      LOG_NOTE("shared input locked; hold Esc for 3 seconds to unlock");
      break;
    case Phase::Releasing:
      if (state.suppressesLocal()) {
        showStatus(@"Releasing input lock…\nRelease all keys and mouse buttons");
      } else {
        showStatus(
            state.awaitingRemoteRelease()
                ? @"Input lock ended; sharing paused\nRemote release failed; check client before restarting"
            : held.historyLost() ? @"Input lock ended; sharing paused\nCapture was interrupted; restart Deskflow"
                                 : @"Input lock ended; sharing paused\nRelease all keys and mouse buttons"
        );
        restoreState(false);
      }
      LOG_NOTE("releasing shared input lock (reason=%d)", static_cast<int>(state.reason()));
      break;
    }
  }

  bool releaseLocal()
  {
    CGEventSourceRef source = CGEventSourceCreate(kCGEventSourceStatePrivate);
    if (!source) {
      return false;
    }
    CGEventSourceSetLocalEventsSuppressionInterval(source, 0);
    const auto flags = CGEventSourceFlagsState(kCGEventSourceStateCombinedSessionState) & kCGEventFlagMaskAlphaShift;
    bool success = true;
    // Release ordinary keys before modifiers. Session posting bypasses our
    // HID tap; a private source leaves the physical HID state table untouched.
    for (bool modifiers : {false, true}) {
      for (CGKeyCode key = 0; key < localKeys.size(); ++key) {
        if (!localKeys[key] || isModifier(key) != modifiers) {
          continue;
        }
        CGEventRef up = CGEventCreateKeyboardEvent(source, key, false);
        if (!up) {
          success = false;
          continue;
        }
        if (isModifier(key)) {
          CGEventSetType(up, kCGEventFlagsChanged);
        }
        CGEventSetFlags(up, flags);
        CGEventPost(kCGSessionEventTap, up);
        CFRelease(up);
      }
    }
    CGEventRef positionEvent = CGEventCreate(source);
    if (!positionEvent) {
      success = false;
    } else {
      const auto position = CGEventGetLocation(positionEvent);
      for (unsigned button = 0; button < localButtons.size(); ++button) {
        if (!localButtons[button]) {
          continue;
        }
        const auto type = button == 0 ? kCGEventLeftMouseUp : button == 1 ? kCGEventRightMouseUp : kCGEventOtherMouseUp;
        CGEventRef up = CGEventCreateMouseEvent(source, type, position, static_cast<CGMouseButton>(button));
        if (!up) {
          success = false;
          continue;
        }
        CGEventSetFlags(up, flags);
        CGEventPost(kCGSessionEventTap, up);
        CFRelease(up);
      }
      CFRelease(positionEvent);
    }
    for (const auto key : localMedia) {
      NSEvent *up = [NSEvent otherEventWithType:NSEventTypeSystemDefined
                                       location:NSZeroPoint
                                  modifierFlags:0xb00
                                      timestamp:0
                                   windowNumber:0
                                        context:nil
                                        subtype:8
                                          data1:(static_cast<int>(key) << 16) | 0xb00
                                          data2:-1];
      CGEventRef event = [up CGEvent];
      if (event) {
        CGEventRef copy = CGEventCreateCopy(event);
        if (copy) {
          CGEventSetSource(copy, source);
          CGEventPost(kCGSessionEventTap, copy);
          CFRelease(copy);
        } else {
          success = false;
        }
      } else {
        success = false;
      }
    }
    CFRelease(source);
    if (success) {
      localKeys.reset();
      localButtons.reset();
      localMedia.clear();
    }
    return success;
  }

  void request(unsigned seconds)
  {
    // A main-queue request may outlive screen shutdown. It must not recreate
    // the status panel or acquire capture resources after stop().
    if (!enabled) {
      return;
    }
    if (state.phase() == Phase::Open && held.historyLost()) {
      showStatus(@"Input lock unavailable\nCapture was interrupted; restart Deskflow");
      LOG_WARN("input lock requires a new capture session after input history was lost");
      return;
    }
    if (state.phase() != Phase::Open || !healthy() || !startTimer() ||
        !state.begin(State::Clock::now(), true, std::chrono::seconds(seconds))) {
      LOG_WARN("shared input lock unavailable (capture disabled, secure input active, or recovery pending)");
      return;
    }
    updateStatus();
    // Release only input visible locally at capture startup or subsequently
    // observed on the local screen. A remote-only hold must not create a
    // local synthetic up, and Quartz cannot repair our filtered history.
    const bool released = releaseLocal();
    // This producer posts after closing capture, behind all earlier physical
    // input. No newly captured input is queued until resume is acknowledged.
    post(deskflow::EventTypes::SharedInputLockPrepare);
    if (released) {
      state.localReleased(state.generation(), State::Clock::now());
    } else {
      state.release(Reason::CaptureLost, State::Clock::now());
    }
    tick();
  }

  void tick()
  {
    if (!enabled) {
      return;
    }
    if (state.phase() == Phase::Open) {
      stopTimer();
      updateStatus();
      return;
    }
    const bool captureHealthy = healthy();
    if (!captureHealthy) {
      held.invalidateHistory();
    }
    // Swallowed hardware events do not update Quartz's source-state table.
    // Keep the observed Esc hold until its up event; capture loss invalidates
    // the whole history and prevents the resume barrier from opening sharing.
    state.update(State::Clock::now(), captureHealthy, held.keyHeld(kVK_Escape));
    if (state.phase() == Phase::Releasing && state.requestResumeBarrier(inputsReleased())) {
      post(deskflow::EventTypes::SharedInputLockResume);
    }
    updateStatus();
  }

  void start(CFMachPortRef newTap)
  {
    if (enabled || !newTap) {
      return;
    }
    state.resetAfterStop();
    localKeys.reset();
    localButtons.reset();
    held = {};
    held.seedHid(
        [](unsigned key) { return CGEventSourceKeyState(kCGEventSourceStateHIDSystemState, key); },
        [](unsigned button) {
          return CGEventSourceButtonState(kCGEventSourceStateHIDSystemState, static_cast<CGMouseButton>(button));
        }
    );
    // The primary screen starts locally. Keep its initial visible input
    // separate from later holds observed while sharing to a remote screen.
    localKeys = held.hidKeys;
    localButtons = held.hidButtons;
    localMedia.clear();
    CFRetain(newTap);
    tap = newTap;
    enabled = true;
  }

  bool startTimer()
  {
    if (timer) {
      return true;
    }
    CFRunLoopTimerContext context = {0, this, nullptr, nullptr, nullptr};
    timer = CFRunLoopTimerCreate(
        kCFAllocatorDefault, CFAbsoluteTimeGetCurrent() + 0.05, 0.05, 0, 0,
        [](CFRunLoopTimerRef, void *info) { static_cast<Impl *>(info)->tick(); }, &context
    );
    if (timer) {
      CFRunLoopAddTimer(CFRunLoopGetMain(), timer, kCFRunLoopCommonModes);
    } else {
      LOG_ERR("cannot install shared input lock recovery timer");
    }
    return timer != nullptr;
  }

  void stopTimer()
  {
    if (timer) {
      CFRunLoopTimerInvalidate(timer);
      CFRelease(timer);
      timer = nullptr;
    }
  }

  void stop()
  {
    enabled = false;
    state.release(Reason::Stopped, State::Clock::now());
    stopTimer();
    if (tap) {
      CFRelease(tap);
      tap = nullptr;
    }
    shownPhase = Phase::Open;
    shownReason = Reason::Stopped;
    shownSuppression = false;
    shownAwaitingRemote = false;
    shownHistoryLost = false;
    if (panel) {
      [panel close];
      [panel release];
      panel = nil;
      label = nil;
    }
    // The owning screen is shutting down: its server handlers are removed
    // before this call. Do not reopen the forwarding gate on a dying screen.
  }

  FilterResult filter(CGEventType type, CGEventRef event, bool local)
  {
    if (!enabled) {
      return state.blocksForwarding() ? FilterResult::LocalOnly : FilterResult::Pass;
    }
    if (type == kCGEventTapDisabledByTimeout || type == kCGEventTapDisabledByUserInput) {
      held.invalidateHistory();
      state.release(Reason::CaptureLost, State::Clock::now());
      tick();
      return FilterResult::Pass; // allow the existing tap recovery handler
    }
    // An up from one source must not erase another source's held input.
    const bool fromHid = CGEventGetIntegerValueField(event, kCGEventSourceStateID) == kCGEventSourceStateHIDSystemState;
    if (type == kCGEventKeyDown || type == kCGEventKeyUp || type == kCGEventFlagsChanged) {
      held.key(
          static_cast<unsigned>(CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode)), type,
          CGEventGetFlags(event), fromHid
      );
    } else if (type == kCGEventLeftMouseDown || type == kCGEventRightMouseDown || type == kCGEventOtherMouseDown ||
               type == kCGEventLeftMouseUp || type == kCGEventRightMouseUp || type == kCGEventOtherMouseUp) {
      held.button(
          static_cast<unsigned>(CGEventGetIntegerValueField(event, kCGMouseEventButtonNumber)),
          type == kCGEventLeftMouseDown || type == kCGEventRightMouseDown || type == kCGEventOtherMouseDown, fromHid
      );
    }
    // Track media key releases even while swallowing events. These are not
    // represented in CGEventSourceKeyState's virtual keyboard table.
    if (type == static_cast<CGEventType>(NSEventTypeSystemDefined)) {
      @try {
        NSEvent *native = [NSEvent eventWithCGEvent:event];
        if ([native subtype] == 8) {
          const auto data = static_cast<uint32_t>([native data1]);
          const auto key = static_cast<uint16_t>(data >> 16);
          const bool down = (data & 0x100) == 0;
          held.mediaKey(key, down);
          if (down) {
            if (local && !state.blocksForwarding()) {
              localMedia.insert(key);
            }
          } else {
            localMedia.erase(key);
          }
        }
      } @catch (NSException *) {
        held.invalidateHistory();
        if (state.blocksForwarding()) {
          state.release(Reason::CaptureLost, State::Clock::now());
        }
      }
    }
    if (state.blocksForwarding()) {
      if ((type == kCGEventKeyDown || type == kCGEventKeyUp) &&
          CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode) == kVK_Escape) {
        state.escape(
            type == kCGEventKeyDown, CGEventGetIntegerValueField(event, kCGKeyboardEventAutorepeat) != 0,
            State::Clock::now()
        );
      }
      // Health is checked here too; do not wait for a timer before removing a
      // stale Locked indication. Never run onKey/onMouse or post input here.
      tick();
      return state.suppressesLocal() ? FilterResult::Suppress : FilterResult::LocalOnly;
    }
    if (local) {
      switch (type) {
      case kCGEventKeyDown:
      case kCGEventKeyUp:
      case kCGEventFlagsChanged: {
        const auto key = CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
        if (key >= 0 && key < static_cast<int64_t>(localKeys.size()) && !isModifier(static_cast<CGKeyCode>(key))) {
          localKeys[key] = type == kCGEventKeyDown;
        }
        updateInputLockModifiers(localKeys, CGEventGetFlags(event));
        break;
      }
      case kCGEventLeftMouseDown:
      case kCGEventRightMouseDown:
      case kCGEventOtherMouseDown:
      case kCGEventLeftMouseUp:
      case kCGEventRightMouseUp:
      case kCGEventOtherMouseUp: {
        const auto button = CGEventGetIntegerValueField(event, kCGMouseEventButtonNumber);
        if (button >= 0 && button < static_cast<int64_t>(localButtons.size())) {
          localButtons[button] =
              type == kCGEventLeftMouseDown || type == kCGEventRightMouseDown || type == kCGEventOtherMouseDown;
        }
        break;
      }
      default:
        break;
      }
    }
    return FilterResult::Pass;
  }

  IEventQueue *events;
  void *target;
  std::function<void(bool)> restoreState;
  State state;
  CFMachPortRef tap = nullptr;
  CFRunLoopTimerRef timer = nullptr;
  bool enabled = false;
  std::bitset<128> localKeys;
  std::bitset<32> localButtons;
  OSXInputLockHeldInputs held;
  std::set<uint16_t> localMedia;
  NSPanel *panel = nil;
  NSTextField *label = nil;
  Phase shownPhase = Phase::Open;
  Reason shownReason = Reason::None;
  bool shownSuppression = false;
  bool shownAwaitingRemote = false;
  bool shownHistoryLost = false;
};

OSXInputLock::OSXInputLock(IEventQueue *events, void *target, std::function<void(bool)> restoreState)
    : m_impl(std::make_shared<Impl>(events, target, std::move(restoreState)))
{
}

OSXInputLock::~OSXInputLock()
{
  stop();
}

void OSXInputLock::start(CFMachPortRef tap)
{
  const auto impl = m_impl;
  onMainSync(^{
    impl->start(tap);
  });
}

void OSXInputLock::stop()
{
  const auto impl = m_impl;
  onMainSync(^{
    impl->stop();
  });
}

void OSXInputLock::request(unsigned autoReleaseSeconds)
{
  const auto impl = m_impl;
  dispatch_async(dispatch_get_main_queue(), ^{
    impl->request(autoReleaseSeconds);
  });
}

void OSXInputLock::confirm(uint64_t generation, bool success)
{
  const auto impl = m_impl;
  dispatch_async(dispatch_get_main_queue(), ^{
    if (!impl->enabled || impl->state.generation() != generation) {
      return;
    }
    if (success) {
      // Test capture health before acknowledging a late network reply.
      impl->tick();
      impl->state.remoteReleased(generation, State::Clock::now());
    } else {
      impl->state.release(Reason::RemoteUnavailable, State::Clock::now());
    }
    impl->tick();
  });
}

void OSXInputLock::resume(uint64_t generation)
{
  const auto impl = m_impl;
  dispatch_async(dispatch_get_main_queue(), ^{
    if (!impl->enabled || impl->state.generation() != generation) {
      return;
    }
    // All physical keys are up here, including the unlocking Esc. Reset the
    // platform's logical key/button state before reopening the shared gate.
    const bool released = impl->inputsReleased();
    if (released) {
      impl->restoreState(true);
    }
    impl->state.acknowledgeResume(generation, released);
    impl->tick();
  });
}

void OSXInputLock::cancel()
{
  const auto impl = m_impl;
  dispatch_async(dispatch_get_main_queue(), ^{
    if (impl->enabled) {
      impl->state.release(Reason::Stopped, State::Clock::now());
      impl->tick();
    }
  });
}

bool OSXInputLock::blocksForwarding() const
{
  return m_impl->state.blocksForwarding();
}

OSXInputLock::FilterResult OSXInputLock::filter(CGEventType type, CGEventRef event, bool onScreen)
{
  return m_impl->filter(type, event, onScreen);
}

/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/SharedInputLock.h"

#ifdef __APPLE__
#include "platform/OSXInputLockHeldInputs.h"
#endif

#include <iostream>
#include <stdexcept>
#include <string>

using Lock = deskflow::SharedInputLock;
using Phase = Lock::Phase;
using Reason = Lock::Reason;
using namespace std::chrono_literals;

static const Lock::Time start{};

static void require(bool result, const char *message)
{
  if (!result) {
    throw std::runtime_error(message);
  }
}

static void arm(Lock &lock, Lock::Duration duration = Lock::Duration::zero())
{
  require(lock.begin(start, true, duration), "begin rejected");
  lock.localReleased(lock.generation(), start);
  lock.remoteReleased(lock.generation(), start);
  require(lock.phase() == Phase::Locked, "not locked after both release acknowledgements");
}

static void preparation()
{
  Lock lock;
  require(!lock.begin(start, false), "unhealthy capture accepted");
  require(!lock.blocksForwarding() && !lock.suppressesLocal(), "failed begin changed gates");
  require(lock.begin(start, true), "healthy capture rejected");
  const auto generation = lock.generation();
  require(lock.blocksForwarding() && lock.suppressesLocal(), "preparation did not close gates");
  lock.remoteReleased(generation, start + 1ms);
  require(lock.phase() == Phase::Preparing, "locked before local releases");
  lock.localReleased(generation - 1, start + 2ms);
  require(lock.phase() == Phase::Preparing, "stale generation armed lock");
  require(!lock.begin(start + 3ms, true), "duplicate begin accepted");
  require(lock.generation() == generation, "duplicate begin replaced generation");
  lock.localReleased(generation, start + 4ms);
  require(lock.phase() == Phase::Locked, "release acknowledgements did not arm lock");
}

static void escapeTiming()
{
  Lock lock;
  arm(lock);
  lock.escape(true, true, start);
  lock.update(start + 4s, true, true);
  require(lock.phase() == Phase::Locked, "preexisting repeat started Esc hold");
  lock.escape(true, false, start + 5s);
  lock.escape(true, true, start + 6s);
  lock.escape(true, false, start + 7s);
  lock.update(start + 7999ms, true, true);
  require(lock.phase() == Phase::Locked, "Esc released before three seconds");
  lock.update(start + 8s, true, true);
  require(lock.phase() == Phase::Releasing && lock.reason() == Reason::Escape, "Esc hold was reset by repeat");
  require(lock.blocksForwarding(), "Esc reopened forwarding without draining");
  require(!lock.requestResumeBarrier(false), "Esc still held but recovery allowed");
  require(lock.requestResumeBarrier(true), "released inputs did not request queue boundary");
  require(!lock.requestResumeBarrier(true), "duplicate queue boundary requested");
  require(lock.acknowledgeResume(lock.generation(), true), "queue boundary did not finish release");
  require(!lock.blocksForwarding() && !lock.suppressesLocal(), "release left input blocked");
}

static void interruptedEscape()
{
  Lock lock;
  arm(lock);
  lock.escape(true, false, start);
  lock.escape(false, false, start + 2999ms);
  lock.update(start + 3s, true, false);
  require(lock.phase() == Phase::Locked, "short Esc press unlocked");
  lock.escape(true, false, start + 4s);
  lock.update(start + 5s, true, false); // adapter supplies an observed release
  lock.update(start + 10s, true, true);
  require(lock.phase() == Phase::Locked, "missing Esc up left hold armed");
}

static void queueBoundaryAndNewPress()
{
  Lock lock;
  arm(lock, 10s);
  lock.update(start + 10s, true, false);
  require(lock.reason() == Reason::AutoRelease, "automatic release missed deadline");
  require(!lock.acknowledgeResume(lock.generation(), true), "unsolicited resume acknowledged");
  require(lock.requestResumeBarrier(true), "barrier not requested");
  require(!lock.acknowledgeResume(lock.generation() - 1, true), "stale barrier reopened forwarding");
  require(!lock.acknowledgeResume(lock.generation(), false), "new held input escaped during queue drain");
  require(lock.blocksForwarding(), "forwarding reopened with input held");
  require(lock.requestResumeBarrier(true), "cannot retry after new held input released");
  require(lock.acknowledgeResume(lock.generation(), true), "retry did not release");
  const auto old = lock.generation();
  require(lock.begin(start + 11s, true), "cannot lock again");
  lock.localReleased(old, start + 11s);
  lock.remoteReleased(old, start + 11s);
  require(lock.phase() == Phase::Preparing, "late old acknowledgements armed new generation");
}

static void failureAndBoundedSuppression()
{
  Lock lock;
  arm(lock);
  lock.update(start + 1s, false, false);
  require(lock.phase() == Phase::Releasing && lock.reason() == Reason::CaptureLost, "capture loss still Locked");
  require(!lock.suppressesLocal(), "capture loss claimed suppression");
  require(lock.blocksForwarding(), "capture loss replayed queued input");
  require(!lock.requestResumeBarrier(false), "capture loss resumed sharing with physical input held");
  require(lock.requestResumeBarrier(true), "capture loss cannot drain after physical release");
  require(lock.acknowledgeResume(lock.generation(), true), "fault queue boundary did not recover");

  require(lock.begin(start + 2s, true), "cannot start after failure");
  lock.update(start + 4s, true, false);
  require(lock.reason() == Reason::PrepareTimeout, "preparation has no deadline");
  lock.localReleased(lock.generation(), start + 4s);
  lock.remoteReleased(lock.generation(), start + 4s);
  require(lock.phase() == Phase::Releasing, "late prepare acknowledgement relocked");
  lock.update(start + 8999ms, true, false);
  require(lock.suppressesLocal(), "recovery dropped local suppression too early");
  lock.update(start + 9s, true, false);
  require(!lock.suppressesLocal() && lock.reason() == Reason::ReleaseTimeout, "recovery can trap local input forever");
  require(lock.blocksForwarding(), "recovery timeout bypassed server queue boundary");
}

static void acknowledgementAtDeadline()
{
  Lock lock;
  require(lock.begin(start, true), "begin failed");
  lock.localReleased(lock.generation(), start);
  lock.remoteReleased(lock.generation(), start + 2s);
  require(lock.reason() == Reason::PrepareTimeout, "deadline depends on timer callback ordering");
}

static void unconfirmedRemoteKeepsSharingPaused()
{
  // A missing reply may still arrive after the preparation timeout. This is
  // not an explicit failure, which the server treats as terminal for the round.
  Lock lock;
  require(lock.begin(start, true), "begin failed");
  lock.localReleased(lock.generation(), start);
  lock.update(start + 2s, true, false);
  require(!lock.requestResumeBarrier(true), "unconfirmed remote released sharing");
  lock.update(start + 7s, true, false);
  require(!lock.suppressesLocal(), "remote failure trapped local input");
  require(lock.blocksForwarding() && lock.awaitingRemoteRelease(), "remote failure reopened sharing");
  lock.remoteReleased(lock.generation() - 1, start + 8s);
  require(!lock.requestResumeBarrier(true), "stale remote receipt reopened sharing");
  lock.remoteReleased(lock.generation(), start + 8s);
  require(lock.phase() == Phase::Releasing, "late receipt relocked input");
  require(lock.requestResumeBarrier(true), "late matching receipt cannot recover sharing");
  require(lock.acknowledgeResume(lock.generation(), true), "confirmed remote did not recover");
}

#ifdef __APPLE__
static void modifierEventState()
{
  std::bitset<128> keys;
  keys[kVK_ANSI_A] = true;
  updateInputLockModifiers(keys, kCGEventFlagMaskControl | NX_DEVICELCTLKEYMASK | NX_DEVICERCTLKEYMASK);
  require(keys[kVK_Control] && keys[kVK_RightControl], "both control keys not recorded");
  updateInputLockModifiers(keys, kCGEventFlagMaskControl | NX_DEVICERCTLKEYMASK);
  require(!keys[kVK_Control] && keys[kVK_RightControl], "left up lost the held right modifier");
  updateInputLockModifiers(keys, kCGEventFlagMaskAlphaShift);
  require(!keys[kVK_Control] && !keys[kVK_RightControl] && !keys[kVK_CapsLock], "final modifier up stayed held");
  require(keys[kVK_ANSI_A], "modifier update changed ordinary held key");
  updateInputLockModifiers(keys, kCGEventFlagMaskCommand | kCGEventFlagMaskSecondaryFn);
  require(keys[kVK_Command] && keys[kVK_Function], "aggregate synthetic modifiers not recorded");
  updateInputLockModifiers(keys, 0);
  require(!keys[kVK_Command] && !keys[kVK_Function], "aggregate modifier release stayed held");
}
#endif

static void stopAndNoDefaultDeadline()
{
  Lock lock;
  arm(lock);
  lock.update(start + 24h, true, false);
  require(lock.phase() == Phase::Locked, "default lock unexpectedly expired");
  lock.release(Reason::Stopped, start + 24h);
  require(!lock.suppressesLocal() && lock.blocksForwarding(), "stop did not separate local and queue recovery");
  lock.update(start + 24h + 6s, true, false);
  require(lock.requestResumeBarrier(true), "stop cannot drain");
  require(lock.acknowledgeResume(lock.generation(), true), "stop did not recover");
}

static void escapePressedDuringPreparation()
{
  Lock lock;
  require(lock.begin(start, true), "begin failed");
  lock.localReleased(lock.generation(), start);
  lock.escape(true, false, start + 10ms);
  lock.remoteReleased(lock.generation(), start + 1s);
  lock.escape(true, true, start + 2s);
  lock.update(start + 3999ms, true, true);
  require(lock.phase() == Phase::Locked, "preparation shortened Esc hold");
  lock.update(start + 4s, true, true);
  require(lock.phase() == Phase::Releasing && lock.reason() == Reason::Escape, "fresh preparation Esc was lost");

  Lock released;
  require(released.begin(start, true), "begin failed");
  released.localReleased(released.generation(), start);
  released.escape(true, false, start + 10ms);
  released.escape(false, false, start + 20ms);
  released.remoteReleased(released.generation(), start + 1s);
  released.update(start + 5s, true, true);
  require(released.phase() == Phase::Locked, "released preparation Esc armed a later hold");
}

#ifdef __APPLE__
static void capturedReleasesKeepSourcesIndependent()
{
  OSXInputLockHeldInputs held;
  held.seedHid([](unsigned) { return false; }, [](unsigned) { return false; });
  held.key(kVK_ANSI_A, kCGEventKeyDown, 0, true);
  held.button(0, true, true);
  require(!held.readyToResume(), "captured input was discarded by an initially empty source table");
  held.key(kVK_ANSI_A, kCGEventKeyUp, 0, true);
  require(!held.readyToResume(), "key release also released a held mouse button");
  held.button(0, false, true);
  require(held.readyToResume(), "observed key and button ups did not release input");

  held.seedHid([](unsigned key) { return key == kVK_ANSI_A; }, [](unsigned button) { return button == 0; });
  held.key(kVK_ANSI_A, kCGEventKeyUp, 0, true);
  held.button(0, false, true);
  require(held.readyToResume(), "observed ups did not replace the initial held snapshot");

  held.key(kVK_ANSI_A, kCGEventKeyDown, 0, false);
  held.key(kVK_ANSI_A, kCGEventKeyDown, 0, true);
  held.button(0, true, false);
  held.mediaKey(16, true);
  held.key(kVK_ANSI_A, kCGEventKeyUp, 0, true);
  require(held.hasNonHidHold(), "HID release erased a private-source hold");
  held.key(kVK_ANSI_A, kCGEventKeyUp, 0, false);
  held.button(0, false, false);
  require(!held.empty(), "ordinary releases erased a held media key");
  held.mediaKey(16, false);
  require(held.empty(), "observed source releases could not recover");
}

static void capturedEscapeSurvivesFilteredSourceState()
{
  // Real HID-head filtering leaves Quartz's source table false while a
  // physical Esc is held. Replay the captured transitions into the lock.
  OSXInputLockHeldInputs held;
  held.seedHid([](unsigned) { return false; }, [](unsigned) { return false; });
  Lock lock;
  arm(lock, 30s);
  held.key(kVK_Escape, kCGEventKeyDown, 0, true);
  lock.escape(true, false, start);
  lock.update(start + 2999ms, true, held.keyHeld(kVK_Escape));
  require(lock.phase() == Phase::Locked, "captured Esc released too early");
  lock.escape(true, true, start + 2999ms);
  lock.update(start + 3s, true, held.keyHeld(kVK_Escape));
  require(lock.reason() == Reason::Escape, "filtered source state erased the captured Esc hold");
  require(!lock.requestResumeBarrier(held.readyToResume()), "still-held Esc reopened sharing");
  held.key(kVK_Escape, kCGEventKeyUp, 0, true);
  lock.escape(false, false, start + 4s);
  require(lock.requestResumeBarrier(held.readyToResume()), "captured Esc up did not permit the queue barrier");
  require(lock.acknowledgeResume(lock.generation(), held.readyToResume()), "Esc release did not resume sharing");
}

static void captureLossRequiresNewHistory()
{
  OSXInputLockHeldInputs held;
  held.key(kVK_ANSI_A, kCGEventKeyDown, 0, true);
  held.invalidateHistory();
  held.key(kVK_ANSI_A, kCGEventKeyUp, 0, true);
  require(held.empty(), "observed up did not clear the known key");
  require(!held.readyToResume(), "known releases falsely repaired missing capture history");
  held.seedHid([](unsigned) { return false; }, [](unsigned) { return false; });
  require(!held.readyToResume(), "Quartz snapshot falsely repaired missing capture history");
  held = {};
  held.seedHid([](unsigned) { return false; }, [](unsigned) { return false; });
  require(held.readyToResume(), "new capture history could not start");
}

static void modifierFlagsDoNotInventSourceHolds()
{
  OSXInputLockHeldInputs held;
  const auto shift = kCGEventFlagMaskShift | NX_DEVICELSHIFTKEYMASK;
  held.key(kVK_ANSI_A, kCGEventKeyDown, shift, false);
  held.key(kVK_ANSI_A, kCGEventKeyUp, shift, false);
  held.key(kVK_Shift, kCGEventKeyUp, 0, true);
  require(held.empty(), "ordinary automation event invented a private modifier hold");

  held.key(kVK_Shift, kCGEventKeyDown, shift, false);
  held.key(kVK_Shift, kCGEventKeyUp, 0, true);
  held.key(kVK_ANSI_A, kCGEventKeyUp, 0, true);
  require(held.hasNonHidHold(), "another source's flags erased a real private modifier");
  held.key(kVK_Shift, kCGEventKeyUp, 0, false);
  require(held.empty(), "private modifier release was not observed");

  held.key(kVK_RightShift, kCGEventFlagsChanged, kCGEventFlagMaskShift, false);
  held.key(kVK_Shift, kCGEventFlagsChanged, shift, true);
  held.key(kVK_Shift, kCGEventFlagsChanged, 0, true);
  require(held.otherKeys[kVK_RightShift] && !held.otherKeys[kVK_Shift], "modifier transition changed the wrong key");
  held.key(kVK_RightShift, kCGEventFlagsChanged, 0, false);
  require(held.empty(), "flags-changed release failed to clear its source's modifier");
}
#endif

static void stoppedSessionCanBeEnabledAgain()
{
  Lock lock;
  arm(lock);
  const auto old = lock.generation();
  lock.release(Reason::Stopped, start + 1s);
  lock.resetAfterStop();
  require(!lock.blocksForwarding() && lock.phase() == Phase::Open, "reenabled screen stayed blocked");
  require(lock.begin(start + 2s, true), "reenabled capture could not lock");
  lock.localReleased(old, start + 2s);
  lock.remoteReleased(old, start + 2s);
  require(lock.phase() == Phase::Preparing, "retired capture callback rearmed new screen");
}

int main()
{
  const std::pair<const char *, void (*)()> tests[] = {
      {"preparation", preparation},
      {"escapeTiming", escapeTiming},
      {"interruptedEscape", interruptedEscape},
      {"queueBoundaryAndNewPress", queueBoundaryAndNewPress},
      {"failureAndBoundedSuppression", failureAndBoundedSuppression},
      {"acknowledgementAtDeadline", acknowledgementAtDeadline},
      {"unconfirmedRemoteKeepsSharingPaused", unconfirmedRemoteKeepsSharingPaused},
#ifdef __APPLE__
      {"modifierEventState", modifierEventState},
#endif
      {"stopAndNoDefaultDeadline", stopAndNoDefaultDeadline},
      {"escapePressedDuringPreparation", escapePressedDuringPreparation},
#ifdef __APPLE__
      {"capturedReleasesKeepSourcesIndependent", capturedReleasesKeepSourcesIndependent},
      {"capturedEscapeSurvivesFilteredSourceState", capturedEscapeSurvivesFilteredSourceState},
      {"captureLossRequiresNewHistory", captureLossRequiresNewHistory},
      {"modifierFlagsDoNotInventSourceHolds", modifierFlagsDoNotInventSourceHolds},
#endif
      {"stoppedSessionCanBeEnabledAgain", stoppedSessionCanBeEnabledAgain}
  };
  unsigned failures = 0;
  for (const auto &[name, test] : tests) {
    try {
      test();
      std::cout << "PASS " << name << '\n';
    } catch (const std::exception &e) {
      ++failures;
      std::cerr << "FAIL " << name << ": " << e.what() << '\n';
    }
  }
  return failures == 0 ? 0 : 1;
}

/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "coordination/KeyboardRelayMap.h"

#include "deskflow/KeyTypes.h"
#include "platform/OSXAutoTypes.h"
#include "platform/OSXMainQueue.h"

#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>

namespace deskflow::coordination {

namespace {

KeyModifierMask mapModifiers(CGEventFlags flags)
{
  KeyModifierMask mask = 0;
  if ((flags & kCGEventFlagMaskShift) != 0) {
    mask |= shiftKey;
  }
  if ((flags & kCGEventFlagMaskControl) != 0) {
    mask |= controlKey;
  }
  if ((flags & kCGEventFlagMaskCommand) != 0) {
    mask |= cmdKey;
  }
  if ((flags & kCGEventFlagMaskAlternate) != 0) {
    mask |= optionKey;
  }
  if ((flags & kCGEventFlagMaskAlphaShift) != 0) {
    mask |= alphaLock;
  }
  return mask;
}

KeyID translateVirtualKey(CGKeyCode vk, const UCKeyboardLayout *layout, UInt32 keyboardType)
{
  switch (vk) {
  case kVK_Return:
    return kKeyReturn;
  case kVK_Tab:
    return kKeyTab;
  case kVK_Space:
    return static_cast<KeyID>(' ');
  case kVK_Delete:
    return kKeyBackSpace;
  case kVK_ForwardDelete:
    return kKeyDelete;
  case kVK_Escape:
    return kKeyEscape;
  case kVK_LeftArrow:
    return kKeyLeft;
  case kVK_RightArrow:
    return kKeyRight;
  case kVK_UpArrow:
    return kKeyUp;
  case kVK_DownArrow:
    return kKeyDown;
  case kVK_Home:
    return kKeyHome;
  case kVK_End:
    return kKeyEnd;
  case kVK_PageUp:
    return kKeyPageUp;
  case kVK_PageDown:
    return kKeyPageDown;
  default:
    break;
  }

  if (layout == nullptr) {
    return kKeyNone;
  }

  UInt32 deadKeyState = 0;
  UniChar chars[4];
  UniCharCount count = 0;
  if (UCKeyTranslate(
          layout, vk, kUCKeyActionDisplay, 0, keyboardType, kUCKeyTranslateNoDeadKeysMask, &deadKeyState, 4, &count,
          chars
      ) == noErr &&
      count > 0) {
    const UniChar c = chars[0];
    if (c >= 32) {
      return static_cast<KeyID>(c);
    }
  }
  return kKeyNone;
}

} // namespace

bool mapRelayKeyFromCgEvent(
    void *cgEvent, Message::KeyPhase &phase, KeyID &id, KeyModifierMask &mask, KeyButton &button
)
{
  auto *event = static_cast<CGEventRef>(cgEvent);
  const CGEventType type = CGEventGetType(event);
  if (type == kCGEventKeyDown) {
    phase = CGEventGetIntegerValueField(event, kCGKeyboardEventAutorepeat) != 0 ? Message::KeyPhase::Repeat
                                                                               : Message::KeyPhase::Down;
  } else if (type == kCGEventKeyUp) {
    phase = Message::KeyPhase::Up;
  } else {
    return false;
  }

  const CGKeyCode vk = static_cast<CGKeyCode>(CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode));
  button = static_cast<KeyButton>(vk);
  mask = mapModifiers(CGEventGetFlags(event));

  if (type == kCGEventKeyUp) {
    id = kKeyNone;
    return true;
  }

  // TIS APIs assert the main dispatch queue on macOS 14+; relay tap runs on a
  // dedicated CFRunLoop thread (see OSXKeyboardRelayMonitor).
  const UInt32 keyboardType = LMGetKbdType();
  CFDataRef layoutRef = deskflow::platform::osx::runOnMainQueue([]() -> CFDataRef {
    std::lock_guard<std::mutex> lock(g_tisMutex);
    AutoTISInputSourceRef source(TISCopyCurrentKeyboardInputSource(), CFRelease);
    if (!source) {
      return nullptr;
    }
    CFDataRef ref = static_cast<CFDataRef>(TISGetInputSourceProperty(source.get(), kTISPropertyUnicodeKeyLayoutData));
    if (ref) {
      CFRetain(ref);
    }
    return ref;
  });
  AutoCFData layoutData(layoutRef, CFRelease);
  const auto *layout =
      layoutData ? reinterpret_cast<const UCKeyboardLayout *>(CFDataGetBytePtr(layoutData.get())) : nullptr;
  id = translateVirtualKey(vk, layout, keyboardType);

  return id != kKeyNone || phase == Message::KeyPhase::Repeat;
}

} // namespace deskflow::coordination

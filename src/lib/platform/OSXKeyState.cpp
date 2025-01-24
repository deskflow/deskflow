/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/OSXKeyState.h"
#include "arch/Arch.h"
#include "base/Log.h"
#include "platform/OSXMediaKeySupport.h"
#include "platform/OSXUchrKeyResource.h"

#include <Carbon/Carbon.h>
#include <IOKit/hidsystem/IOHIDLib.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

// Note that some virtual keys codes appear more than once.  The
// first instance of a virtual key code maps to the KeyID that we
// want to generate for that code.  The others are for mapping
// different KeyIDs to a single key code.
static const uint32_t s_shiftVK = kVK_Shift;
static const uint32_t s_controlVK = kVK_Control;
static const uint32_t s_altVK = kVK_Option;
static const uint32_t s_superVK = kVK_Command;
static const uint32_t s_capsLockVK = kVK_CapsLock;
static const uint32_t s_numLockVK = kVK_ANSI_KeypadClear; // 71

static const uint32_t s_brightnessUp = 144;
static const uint32_t s_brightnessDown = 145;
static const uint32_t s_missionControlVK = 160;
static const uint32_t s_launchpadVK = 131;

static const uint32_t s_osxNumLock = 1 << 16;

struct KeyEntry
{
public:
  KeyID m_keyID;
  uint32_t m_virtualKey;
};
static const KeyEntry s_controlKeys[] = {
    // cursor keys.  if we don't do this we'll may still get these from
    // the keyboard resource but they may not correspond to the arrow
    // keys.
    {kKeyLeft, kVK_LeftArrow},
    {kKeyRight, kVK_RightArrow},
    {kKeyUp, kVK_UpArrow},
    {kKeyDown, kVK_DownArrow},
    {kKeyHome, kVK_Home},
    {kKeyEnd, kVK_End},
    {kKeyPageUp, kVK_PageUp},
    {kKeyPageDown, kVK_PageDown},
    {kKeyInsert, kVK_Help}, // Mac Keyboards have 'Help' on 'Insert'

    // function keys
    {kKeyF1, kVK_F1},
    {kKeyF2, kVK_F2},
    {kKeyF3, kVK_F3},
    {kKeyF4, kVK_F4},
    {kKeyF5, kVK_F5},
    {kKeyF6, kVK_F6},
    {kKeyF7, kVK_F7},
    {kKeyF8, kVK_F8},
    {kKeyF9, kVK_F9},
    {kKeyF10, kVK_F10},
    {kKeyF11, kVK_F11},
    {kKeyF12, kVK_F12},
    {kKeyF13, kVK_F13},
    {kKeyF14, kVK_F14},
    {kKeyF15, kVK_F15},
    {kKeyF16, kVK_F16},

    {kKeyKP_0, kVK_ANSI_Keypad0},
    {kKeyKP_1, kVK_ANSI_Keypad1},
    {kKeyKP_2, kVK_ANSI_Keypad2},
    {kKeyKP_3, kVK_ANSI_Keypad3},
    {kKeyKP_4, kVK_ANSI_Keypad4},
    {kKeyKP_5, kVK_ANSI_Keypad5},
    {kKeyKP_6, kVK_ANSI_Keypad6},
    {kKeyKP_7, kVK_ANSI_Keypad7},
    {kKeyKP_8, kVK_ANSI_Keypad8},
    {kKeyKP_9, kVK_ANSI_Keypad9},
    {kKeyKP_Decimal, kVK_ANSI_KeypadDecimal},
    {kKeyKP_Equal, kVK_ANSI_KeypadEquals},
    {kKeyKP_Multiply, kVK_ANSI_KeypadMultiply},
    {kKeyKP_Add, kVK_ANSI_KeypadPlus},
    {kKeyKP_Divide, kVK_ANSI_KeypadDivide},
    {kKeyKP_Subtract, kVK_ANSI_KeypadMinus},
    {kKeyKP_Enter, kVK_ANSI_KeypadEnter},

    // virtual key 110 is fn+enter and i have no idea what that's supposed
    // to map to.  also the enter key with numlock on is a modifier but i
    // don't know which.

    // modifier keys.  OS X doesn't seem to support right handed versions
    // of modifier keys so we map them to the left handed versions.
    {kKeyShift_L, s_shiftVK},
    {kKeyShift_R, s_shiftVK}, // 60
    {kKeyControl_L, s_controlVK},
    {kKeyControl_R, s_controlVK}, // 62
    {kKeyAlt_L, s_altVK},
    {kKeyAlt_R, s_altVK},
    {kKeySuper_L, s_superVK},
    {kKeySuper_R, s_superVK}, // 61
    {kKeyMeta_L, s_superVK},
    {kKeyMeta_R, s_superVK}, // 61

    // toggle modifiers
    {kKeyNumLock, s_numLockVK},
    {kKeyCapsLock, s_capsLockVK},

    // for Apple Pro JIS Keyboard, map Kana (IME activate) to Henkan (show next
    // IME conversion), and
    // Eisu (IME deactivate) to Zenkaku (IME activation toggle) on Windows
    // Japanese keyboard (OADG109A)
    {kKeyHenkan, kVK_JIS_Kana},
    {kKeyZenkaku, kVK_JIS_Eisu},

    {kKeyMissionControl, s_missionControlVK},
    {kKeyLaunchpad, s_launchpadVK},
    {kKeyBrightnessUp, s_brightnessUp},
    {kKeyBrightnessDown, s_brightnessDown}
};

namespace {

io_connect_t getService(io_iterator_t iter)
{
  io_connect_t service = 0;
  auto nextIterator = IOIteratorNext(iter);

  if (nextIterator) {
    IOServiceOpen(nextIterator, mach_task_self(), kIOHIDParamConnectType, &service);
    IOObjectRelease(nextIterator);
  }

  return service;
}

io_connect_t getEventDriver()
{
  static io_connect_t sEventDrvrRef = 0;

  if (!sEventDrvrRef) {
    // Get master device port
    mach_port_t masterPort = 0;
    if (!IOMasterPort(bootstrap_port, &masterPort)) {
      io_iterator_t iter = 0;
      auto dict = IOServiceMatching(kIOHIDSystemClass);

      if (!IOServiceGetMatchingServices(masterPort, dict, &iter)) {
        sEventDrvrRef = getService(iter);
      } else {
        LOG((CLOG_WARN, "io service not found"));
      }

      IOObjectRelease(iter);
    } else {
      LOG((CLOG_WARN, "couldn't get io master port"));
    }
  }

  return sEventDrvrRef;
}

bool isModifier(uint8_t virtualKey)
{
  static std::set<uint8_t> modifiers{s_shiftVK, s_superVK, s_altVK, s_controlVK, s_capsLockVK};

  return (modifiers.find(virtualKey) != modifiers.end());
}

} // namespace

//
// OSXKeyState
//

OSXKeyState::OSXKeyState(IEventQueue *events, std::vector<std::string> layouts, bool isLangSyncEnabled)
    : KeyState(events, std::move(layouts), isLangSyncEnabled)
{
  init();
}

OSXKeyState::OSXKeyState(
    IEventQueue *events, deskflow::KeyMap &keyMap, std::vector<std::string> layouts, bool isLangSyncEnabled
)
    : KeyState(events, keyMap, std::move(layouts), isLangSyncEnabled)
{
  init();
}

OSXKeyState::~OSXKeyState()
{
}

void OSXKeyState::init()
{
  m_deadKeyState = 0;
  m_shiftPressed = false;
  m_controlPressed = false;
  m_altPressed = false;
  m_superPressed = false;
  m_capsPressed = false;

  // build virtual key map
  for (size_t i = 0; i < sizeof(s_controlKeys) / sizeof(s_controlKeys[0]); ++i) {

    m_virtualKeyMap[s_controlKeys[i].m_virtualKey] = s_controlKeys[i].m_keyID;
  }
}

KeyModifierMask OSXKeyState::mapModifiersFromOSX(uint32_t mask) const
{
  KeyModifierMask outMask = 0;
  if ((mask & kCGEventFlagMaskShift) != 0) {
    outMask |= KeyModifierShift;
  }
  if ((mask & kCGEventFlagMaskControl) != 0) {
    outMask |= KeyModifierControl;
  }
  if ((mask & kCGEventFlagMaskAlternate) != 0) {
    outMask |= KeyModifierAlt;
  }
  if ((mask & kCGEventFlagMaskCommand) != 0) {
    outMask |= KeyModifierSuper;
  }
  if ((mask & kCGEventFlagMaskAlphaShift) != 0) {
    outMask |= KeyModifierCapsLock;
  }
  if ((mask & kCGEventFlagMaskNumericPad) != 0) {
    outMask |= KeyModifierNumLock;
  }

  LOG((CLOG_DEBUG1 "mask=%04x outMask=%04x", mask, outMask));
  return outMask;
}

KeyModifierMask OSXKeyState::mapModifiersToCarbon(uint32_t mask) const
{
  KeyModifierMask outMask = 0;
  if ((mask & kCGEventFlagMaskShift) != 0) {
    outMask |= shiftKey;
  }
  if ((mask & kCGEventFlagMaskControl) != 0) {
    outMask |= controlKey;
  }
  if ((mask & kCGEventFlagMaskCommand) != 0) {
    outMask |= cmdKey;
  }
  if ((mask & kCGEventFlagMaskAlternate) != 0) {
    outMask |= optionKey;
  }
  if ((mask & kCGEventFlagMaskAlphaShift) != 0) {
    outMask |= alphaLock;
  }
  if ((mask & kCGEventFlagMaskNumericPad) != 0) {
    outMask |= s_osxNumLock;
  }

  return outMask;
}

KeyButton OSXKeyState::mapKeyFromEvent(KeyIDs &ids, KeyModifierMask *maskOut, CGEventRef event) const
{
  ids.clear();

  // map modifier key
  if (maskOut != NULL) {
    KeyModifierMask activeMask = getActiveModifiers();
    activeMask &= ~KeyModifierAltGr;
    *maskOut = activeMask;
  }

  // get virtual key
  uint32_t vkCode = CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);

  // handle up events
  uint32_t eventKind = CGEventGetType(event);
  if (eventKind == kCGEventKeyUp) {
    // the id isn't used.  we just need the same button we used on
    // the key press.  note that we don't use or reset the dead key
    // state;  up events should not affect the dead key state.
    ids.push_back(kKeyNone);
    return mapVirtualKeyToKeyButton(vkCode);
  }

  // check for special keys
  VirtualKeyMap::const_iterator i = m_virtualKeyMap.find(vkCode);
  if (i != m_virtualKeyMap.end()) {
    m_deadKeyState = 0;
    ids.push_back(i->second);
    return mapVirtualKeyToKeyButton(vkCode);
  }

  // get keyboard info
  AutoTISInputSourceRef currentKeyboardLayout(TISCopyCurrentKeyboardLayoutInputSource(), CFRelease);

  if (!currentKeyboardLayout) {
    return kKeyNone;
  }

  // get the event modifiers and remove the command and control
  // keys.  note if we used them though.
  // UCKeyTranslate expects old-style Carbon modifiers, so convert.
  uint32_t modifiers;
  modifiers = mapModifiersToCarbon(CGEventGetFlags(event));
  static const uint32_t s_commandModifiers = cmdKey | controlKey | rightControlKey;
  bool isCommand = ((modifiers & s_commandModifiers) != 0);
  modifiers &= ~s_commandModifiers;

  // if we've used a command key then we want the glyph produced without
  // the option key (i.e. the base glyph).
  // if (isCommand) {
  modifiers &= ~optionKey;
  //}

  // choose action
  uint16_t action;
  if (eventKind == kCGEventKeyDown) {
    action = kUCKeyActionDown;
  } else if (CGEventGetIntegerValueField(event, kCGKeyboardEventAutorepeat) == 1) {
    action = kUCKeyActionAutoKey;
  } else {
    return 0;
  }

  // translate via uchr resource
  CFDataRef ref = (CFDataRef)TISGetInputSourceProperty(currentKeyboardLayout.get(), kTISPropertyUnicodeKeyLayoutData);
  const UCKeyboardLayout *layout = (const UCKeyboardLayout *)CFDataGetBytePtr(ref);
  const bool layoutValid = (layout != NULL);

  if (layoutValid) {
    // translate key
    UniCharCount count;
    UniChar chars[2];
    LOG((CLOG_DEBUG2 "modifiers: %08x", modifiers & 0xffu));
    OSStatus status = UCKeyTranslate(
        layout, vkCode & 0xffu, action, (modifiers >> 8) & 0xffu, LMGetKbdType(), 0, &m_deadKeyState,
        sizeof(chars) / sizeof(chars[0]), &count, chars
    );

    // get the characters
    if (status == 0) {
      if (count != 0 || m_deadKeyState == 0) {
        m_deadKeyState = 0;
        for (UniCharCount i = 0; i < count; ++i) {
          ids.push_back(IOSXKeyResource::unicharToKeyID(chars[i]));
        }
        adjustAltGrModifier(ids, maskOut, isCommand);
        return mapVirtualKeyToKeyButton(vkCode);
      }
      return 0;
    }
  }

  return 0;
}

bool OSXKeyState::fakeCtrlAltDel()
{
  // pass keys through unchanged
  return false;
}

bool OSXKeyState::fakeMediaKey(KeyID id)
{
  return fakeNativeMediaKey(id);
}

CGEventFlags OSXKeyState::getModifierStateAsOSXFlags() const
{
  CGEventFlags modifiers = 0;

  if (m_shiftPressed) {
    modifiers |= kCGEventFlagMaskShift;
  }

  if (m_controlPressed) {
    modifiers |= kCGEventFlagMaskControl;
  }

  if (m_altPressed) {
    modifiers |= kCGEventFlagMaskAlternate;
  }

  if (m_superPressed) {
    modifiers |= kCGEventFlagMaskCommand;
  }

  if (m_capsPressed) {
    modifiers |= kCGEventFlagMaskAlphaShift;
  }

  return modifiers;
}

KeyModifierMask OSXKeyState::pollActiveModifiers() const
{
  // falsely assumed that the mask returned by GetCurrentKeyModifiers()
  // was the same as a CGEventFlags (which is what mapModifiersFromOSX
  // expects). patch by Marc
  uint32_t mask = GetCurrentKeyModifiers();
  KeyModifierMask outMask = 0;

  if ((mask & shiftKey) != 0) {
    outMask |= KeyModifierShift;
  }
  if ((mask & controlKey) != 0) {
    outMask |= KeyModifierControl;
  }
  if ((mask & optionKey) != 0) {
    outMask |= KeyModifierAlt;
  }
  if ((mask & cmdKey) != 0) {
    outMask |= KeyModifierSuper;
  }
  if ((mask & alphaLock) != 0) {
    outMask |= KeyModifierCapsLock;
  }
  if ((mask & s_osxNumLock) != 0) {
    outMask |= KeyModifierNumLock;
  }

  LOG((CLOG_DEBUG1 "mask=%04x outMask=%04x", mask, outMask));
  return outMask;
}

int32_t OSXKeyState::pollActiveGroup() const
{
  AutoTISInputSourceRef keyboardLayout(TISCopyCurrentKeyboardLayoutInputSource(), CFRelease);
  CFDataRef id = (CFDataRef)TISGetInputSourceProperty(keyboardLayout.get(), kTISPropertyInputSourceID);

  GroupMap::const_iterator i = m_groupMap.find(id);
  if (i != m_groupMap.end()) {
    return i->second;
  }

  LOG((CLOG_WARN "can't get the active group, use the first group instead"));

  return 0;
}

void OSXKeyState::pollPressedKeys(KeyButtonSet &pressedKeys) const
{
  ::KeyMap km;
  GetKeys(km);
  const uint8_t *m = reinterpret_cast<const uint8_t *>(km);
  for (uint32_t i = 0; i < 16; ++i) {
    for (uint32_t j = 0; j < 8; ++j) {
      if ((m[i] & (1u << j)) != 0) {
        pressedKeys.insert(mapVirtualKeyToKeyButton(8 * i + j));
      }
    }
  }
}

void OSXKeyState::getKeyMap(deskflow::KeyMap &keyMap)
{
  // update keyboard groups
  int32_t numGroups{0};
  if (getGroups(m_groups)) {
    m_groupMap.clear();
    numGroups = CFArrayGetCount(m_groups.get());
    for (int32_t g = 0; g < numGroups; ++g) {
      TISInputSourceRef keyboardLayout = (TISInputSourceRef)CFArrayGetValueAtIndex(m_groups.get(), g);
      CFDataRef id = (CFDataRef)TISGetInputSourceProperty(keyboardLayout, kTISPropertyInputSourceID);
      m_groupMap[id] = g;
    }
  }

  uint32_t keyboardType = LMGetKbdType();
  for (int32_t g = 0; g < numGroups; ++g) {
    // add special keys
    getKeyMapForSpecialKeys(keyMap, g);

    const void *resource;
    bool layoutValid = false;

    // add regular keys
    // try uchr resource first
    TISInputSourceRef keyboardLayout = (TISInputSourceRef)CFArrayGetValueAtIndex(m_groups.get(), g);
    CFDataRef resourceRef = (CFDataRef)TISGetInputSourceProperty(keyboardLayout, kTISPropertyUnicodeKeyLayoutData);

    layoutValid = resourceRef != NULL;
    if (layoutValid)
      resource = CFDataGetBytePtr(resourceRef);

    if (layoutValid) {
      OSXUchrKeyResource uchr(resource, keyboardType);
      if (uchr.isValid()) {
        LOG((CLOG_DEBUG1 "using uchr resource for group %d", g));
        getKeyMap(keyMap, g, uchr);
        continue;
      }
    }

    LOG((CLOG_DEBUG1 "no keyboard resource for group %d", g));
  }
}

CGEventFlags OSXKeyState::getDeviceDependedFlags() const
{
  CGEventFlags modifiers = 0;

  if (m_shiftPressed) {
    modifiers |= NX_DEVICELSHIFTKEYMASK;
  }

  if (m_controlPressed) {
    modifiers |= NX_DEVICELCTLKEYMASK;
  }

  if (m_altPressed) {
    modifiers |= NX_DEVICELALTKEYMASK;
  }

  if (m_superPressed) {
    modifiers |= NX_DEVICELCMDKEYMASK;
  }

  return modifiers;
}

CGEventFlags OSXKeyState::getKeyboardEventFlags() const
{
  // set the event flags for special keys
  // http://tinyurl.com/pxl742y
  CGEventFlags modifiers = getModifierStateAsOSXFlags();

  if (!m_capsPressed) {
    modifiers |= getDeviceDependedFlags();
  }

  return modifiers;
}

void OSXKeyState::setKeyboardModifiers(CGKeyCode virtualKey, bool keyDown)
{
  switch (virtualKey) {
  case s_shiftVK:
    m_shiftPressed = keyDown;
    break;
  case s_controlVK:
    m_controlPressed = keyDown;
    break;
  case s_altVK:
    m_altPressed = keyDown;
    break;
  case s_superVK:
    m_superPressed = keyDown;
    break;
  case s_capsLockVK:
    m_capsPressed = keyDown;
    break;
  default:
    LOG((CLOG_DEBUG1 "the key is not a modifier"));
    break;
  }
}

kern_return_t OSXKeyState::postHIDVirtualKey(uint8_t virtualKey, bool postDown)
{
  NXEventData event;
  bzero(&event, sizeof(NXEventData));
  auto driver = getEventDriver();
  kern_return_t result = KERN_FAILURE;

  if (driver) {
    if (isModifier(virtualKey)) {
      result =
          IOHIDPostEvent(driver, NX_FLAGSCHANGED, {0, 0}, &event, kNXEventDataVersion, getKeyboardEventFlags(), true);
    } else {
      event.key.keyCode = virtualKey;
      const auto eventType = postDown ? NX_KEYDOWN : NX_KEYUP;
      result = IOHIDPostEvent(driver, eventType, {0, 0}, &event, kNXEventDataVersion, 0, false);
    }
  }

  return result;
}

void OSXKeyState::postKeyboardKey(CGKeyCode virtualKey, bool keyDown)
{
  CGEventRef event = CGEventCreateKeyboardEvent(nullptr, virtualKey, keyDown);
  if (event) {
    CGEventSetFlags(event, getKeyboardEventFlags());
    CGEventPost(kCGHIDEventTap, event);
    CFRelease(event);
  } else {
    LOG((CLOG_CRIT "unable to create keyboard event for keystroke"));
  }
}

void OSXKeyState::fakeKey(const Keystroke &keystroke)
{
  switch (keystroke.m_type) {
  case Keystroke::kButton: {
    bool keyDown = keystroke.m_data.m_button.m_press;
    uint32_t client = keystroke.m_data.m_button.m_client;
    KeyButton button = keystroke.m_data.m_button.m_button;
    CGKeyCode virtualKey = mapKeyButtonToVirtualKey(button);

    LOG(
        (CLOG_DEBUG1 "  button=0x%04x virtualKey=0x%04x keyDown=%s client=0x%04x", button, virtualKey,
         keyDown ? "down" : "up", client)
    );

    setKeyboardModifiers(virtualKey, keyDown);
    if (postHIDVirtualKey(virtualKey, keyDown) != KERN_SUCCESS) {
      LOG((CLOG_WARN, "fail to post hid event"));
      postKeyboardKey(virtualKey, keyDown);
    }

    break;
  }

  case Keystroke::kGroup: {
    int32_t group = keystroke.m_data.m_group.m_group;
    if (!keystroke.m_data.m_group.m_restore) {
      if (keystroke.m_data.m_group.m_absolute) {
        LOG((CLOG_DEBUG1 "  group %d", group));
        setGroup(group);
      } else {
        LOG((CLOG_DEBUG1 "  group %+d", group));
        setGroup(getEffectiveGroup(pollActiveGroup(), group));
      }

      if (pollActiveGroup() != group) {
        LOG((CLOG_WARN "failed to set new keyboard layout"));
      }
    }
    break;
  }
  }
}

void OSXKeyState::getKeyMapForSpecialKeys(deskflow::KeyMap &keyMap, int32_t group) const
{
  // special keys are insensitive to modifers and none are dead keys
  deskflow::KeyMap::KeyItem item;
  for (size_t i = 0; i < sizeof(s_controlKeys) / sizeof(s_controlKeys[0]); ++i) {
    const KeyEntry &entry = s_controlKeys[i];
    item.m_id = entry.m_keyID;
    item.m_group = group;
    item.m_button = mapVirtualKeyToKeyButton(entry.m_virtualKey);
    item.m_required = 0;
    item.m_sensitive = 0;
    item.m_dead = false;
    item.m_client = 0;
    deskflow::KeyMap::initModifierKey(item);
    keyMap.addKeyEntry(item);

    if (item.m_lock) {
      // all locking keys are half duplex on OS X
      keyMap.addHalfDuplexButton(item.m_button);
    }
  }

  // note:  we don't special case the number pad keys.  querying the
  // mac keyboard returns the non-keypad version of those keys but
  // a KeyState always provides a mapping from keypad keys to
  // non-keypad keys so we'll be able to generate the characters
  // anyway.
}

bool OSXKeyState::getKeyMap(deskflow::KeyMap &keyMap, int32_t group, const IOSXKeyResource &r) const
{
  if (!r.isValid()) {
    return false;
  }

  // space for all possible modifier combinations
  std::vector<bool> modifiers(r.getNumModifierCombinations());

  // make space for the keys that any single button can synthesize
  std::vector<std::pair<KeyID, bool>> buttonKeys(r.getNumTables());

  // iterate over each button
  deskflow::KeyMap::KeyItem item;
  for (uint32_t i = 0; i < r.getNumButtons(); ++i) {
    item.m_button = mapVirtualKeyToKeyButton(i);

    // the KeyIDs we've already handled
    std::set<KeyID> keys;

    // convert the entry in each table for this button to a KeyID
    for (uint32_t j = 0; j < r.getNumTables(); ++j) {
      buttonKeys[j].first = r.getKey(j, i);
      buttonKeys[j].second = deskflow::KeyMap::isDeadKey(buttonKeys[j].first);
    }

    // iterate over each character table
    for (uint32_t j = 0; j < r.getNumTables(); ++j) {
      // get the KeyID for the button/table
      KeyID id = buttonKeys[j].first;
      if (id == kKeyNone) {
        continue;
      }

      // if we've already handled the KeyID in the table then
      // move on to the next table
      if (keys.count(id) > 0) {
        continue;
      }
      keys.insert(id);

      // prepare item.  the client state is 1 for dead keys.
      item.m_id = id;
      item.m_group = group;
      item.m_dead = buttonKeys[j].second;
      item.m_client = buttonKeys[j].second ? 1 : 0;
      deskflow::KeyMap::initModifierKey(item);
      if (item.m_lock) {
        // all locking keys are half duplex on OS X
        keyMap.addHalfDuplexButton(i);
      }

      // collect the tables that map to the same KeyID.  we know it
      // can't be any earlier tables because of the check above.
      std::set<uint8_t> tables;
      tables.insert(static_cast<uint8_t>(j));
      for (uint32_t k = j + 1; k < r.getNumTables(); ++k) {
        if (buttonKeys[k].first == id) {
          tables.insert(static_cast<uint8_t>(k));
        }
      }

      // collect the modifier combinations that map to any of the
      // tables we just collected
      for (uint32_t k = 0; k < r.getNumModifierCombinations(); ++k) {
        modifiers[k] = (tables.count(r.getTableForModifier(k)) > 0);
      }

      // figure out which modifiers the key is sensitive to.  the
      // key is insensitive to a modifier if for every modifier mask
      // with the modifier bit unset in the modifiers we also find
      // the same mask with the bit set.
      //
      // we ignore a few modifiers that we know aren't important
      // for generating characters.  in fact, we want to ignore any
      // characters generated by the control key.  we don't map
      // those and instead expect the control modifier plus a key.
      uint32_t sensitive = 0;
      for (uint32_t k = 0; (1u << k) < r.getNumModifierCombinations(); ++k) {
        uint32_t bit = (1u << k);
        if ((bit << 8) == cmdKey || (bit << 8) == controlKey || (bit << 8) == rightControlKey) {
          continue;
        }
        for (uint32_t m = 0; m < r.getNumModifierCombinations(); ++m) {
          if (modifiers[m] != modifiers[m ^ bit]) {
            sensitive |= bit;
            break;
          }
        }
      }

      // find each required modifier mask.  the key can be synthesized
      // using any of the masks.
      std::set<uint32_t> required;
      for (uint32_t k = 0; k < r.getNumModifierCombinations(); ++k) {
        if ((k & sensitive) == k && modifiers[k & sensitive]) {
          required.insert(k);
        }
      }

      // now add a key entry for each key/required modifier pair.
      item.m_sensitive = mapModifiersFromOSX(sensitive << 16);
      for (std::set<uint32_t>::iterator k = required.begin(); k != required.end(); ++k) {
        item.m_required = mapModifiersFromOSX(*k << 16);
        keyMap.addKeyEntry(item);
      }
    }
  }

  return true;
}

bool OSXKeyState::mapDeskflowHotKeyToMac(
    KeyID key, KeyModifierMask mask, uint32_t &macVirtualKey, uint32_t &macModifierMask
) const
{
  // look up button for key
  KeyButton button = getButton(key, pollActiveGroup());
  if (button == 0 && key != kKeyNone) {
    return false;
  }
  macVirtualKey = mapKeyButtonToVirtualKey(button);

  // calculate modifier mask
  macModifierMask = 0;
  if ((mask & KeyModifierShift) != 0) {
    macModifierMask |= shiftKey;
  }
  if ((mask & KeyModifierControl) != 0) {
    macModifierMask |= controlKey;
  }
  if ((mask & KeyModifierAlt) != 0) {
    macModifierMask |= cmdKey;
  }
  if ((mask & KeyModifierSuper) != 0) {
    macModifierMask |= optionKey;
  }
  if ((mask & KeyModifierCapsLock) != 0) {
    macModifierMask |= alphaLock;
  }
  if ((mask & KeyModifierNumLock) != 0) {
    macModifierMask |= s_osxNumLock;
  }

  return true;
}

void OSXKeyState::handleModifierKeys(void *target, KeyModifierMask oldMask, KeyModifierMask newMask)
{
  // compute changed modifiers
  KeyModifierMask changed = (oldMask ^ newMask);

  // synthesize changed modifier keys
  if ((changed & KeyModifierShift) != 0) {
    handleModifierKey(target, s_shiftVK, kKeyShift_L, (newMask & KeyModifierShift) != 0, newMask);
  }
  if ((changed & KeyModifierControl) != 0) {
    handleModifierKey(target, s_controlVK, kKeyControl_L, (newMask & KeyModifierControl) != 0, newMask);
  }
  if ((changed & KeyModifierAlt) != 0) {
    handleModifierKey(target, s_altVK, kKeyAlt_L, (newMask & KeyModifierAlt) != 0, newMask);
  }
  if ((changed & KeyModifierSuper) != 0) {
    handleModifierKey(target, s_superVK, kKeySuper_L, (newMask & KeyModifierSuper) != 0, newMask);
  }
  if ((changed & KeyModifierCapsLock) != 0) {
    handleModifierKey(target, s_capsLockVK, kKeyCapsLock, (newMask & KeyModifierCapsLock) != 0, newMask);
  }
  if ((changed & KeyModifierNumLock) != 0) {
    handleModifierKey(target, s_numLockVK, kKeyNumLock, (newMask & KeyModifierNumLock) != 0, newMask);
  }
}

void OSXKeyState::handleModifierKey(void *target, uint32_t virtualKey, KeyID id, bool down, KeyModifierMask newMask)
{
  KeyButton button = mapVirtualKeyToKeyButton(virtualKey);
  onKey(button, down, newMask);
  sendKeyEvent(target, down, false, id, newMask, 0, button);
}

bool OSXKeyState::getGroups(AutoCFArray &groups) const
{
  // get number of layouts
  CFStringRef keys[] = {kTISPropertyInputSourceCategory};
  CFStringRef values[] = {kTISCategoryKeyboardInputSource};
  AutoCFDictionary dict(CFDictionaryCreate(NULL, (const void **)keys, (const void **)values, 1, NULL, NULL), CFRelease);
  AutoCFArray kbds(TISCreateInputSourceList(dict.get(), false), CFRelease);

  if (CFArrayGetCount(kbds.get()) > 0) {
    groups = std::move(kbds);
  } else {
    LOG((CLOG_DEBUG1 "can't get keyboard layouts"));
    return false;
  }

  return true;
}

void OSXKeyState::setGroup(int32_t group)
{
  TISInputSourceRef keyboardLayout = (TISInputSourceRef)CFArrayGetValueAtIndex(m_groups.get(), group);
  if (!keyboardLayout) {
    LOG((CLOG_WARN "nedeed keyboard layout is null"));
    return;
  }
  auto canBeSetted = (CFBooleanRef
  )TISGetInputSourceProperty(TISCopyCurrentKeyboardInputSource(), kTISPropertyInputSourceIsEnableCapable);
  if (!canBeSetted) {
    LOG((CLOG_WARN "nedeed keyboard layout is disabled for programmatically selection"));
    return;
  }

  if (TISSelectInputSource(keyboardLayout) != noErr) {
    LOG((CLOG_WARN "failed to set nedeed keyboard layout"));
  }

  LOG((CLOG_DEBUG1 "keyboard layout change to %d", group));

  // A minimal delay is needed after a group change because the
  // keyboard key event often happens immediately after.
  // Language (TIS) and event (CG) systems are not in the mutual
  // event queue and without a delay the subsequent key press
  // event could be applied before the keyboard layout would
  // actually be changed.
  ARCH->sleep(.01);
}

void OSXKeyState::adjustAltGrModifier(const KeyIDs &ids, KeyModifierMask *mask, bool isCommand) const
{
  if (!isCommand) {
    for (KeyIDs::const_iterator i = ids.begin(); i != ids.end(); ++i) {
      KeyID id = *i;
      if (id != kKeyNone && ((id < 0xe000u || id > 0xefffu) || (id >= kKeyKP_Equal && id <= kKeyKP_9))) {
        *mask |= KeyModifierAltGr;
        return;
      }
    }
  }
}

KeyButton OSXKeyState::mapVirtualKeyToKeyButton(uint32_t keyCode)
{
  // 'A' maps to 0 so shift every id
  return static_cast<KeyButton>(keyCode + KeyButtonOffset);
}

uint32_t OSXKeyState::mapKeyButtonToVirtualKey(KeyButton keyButton)
{
  return static_cast<uint32_t>(keyButton - KeyButtonOffset);
}

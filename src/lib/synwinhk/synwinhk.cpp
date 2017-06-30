/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2002 Chris Schoeneman
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

#include "synwinhk/synwinhk.h"

#include "synergy/protocol_types.h"

#include <zmouse.h>
#include <tchar.h>

#if _MSC_VER >= 1400
// VS2005 hack - we don't use assert here because we don't want to link with the
// CRT.
#undef assert
#if _DEBUG
#define assert(_X_)                                                            \
    if (!(_X_))                                                                \
    __debugbreak ()
#else
#define assert(_X_) __noop ()
#endif
// VS2005 is a bit more smart than VC6 and optimize simple copy loop to
// intrinsic memcpy.
#pragma function(memcpy)
#endif

//
// debugging compile flag.  when not zero the server doesn't grab
// the keyboard when the mouse leaves the server screen.  this
// makes it possible to use the debugger (via the keyboard) when
// all user input would normally be caught by the hook procedures.
//
#define NO_GRAB_KEYBOARD 0

//
// debugging compile flag.  when not zero the server will not
// install low level hooks.
//
#define NO_LOWLEVEL_HOOKS 0

//
// extra mouse wheel stuff
//

enum EWheelSupport { kWheelNone, kWheelOld, kWheelWin2000, kWheelModern };

// declare extended mouse hook struct.  useable on win2k
typedef struct tagMOUSEHOOKSTRUCTWin2000 {
    MOUSEHOOKSTRUCT mhs;
    DWORD mouseData;
} MOUSEHOOKSTRUCTWin2000;

#if !defined(SM_MOUSEWHEELPRESENT)
#define SM_MOUSEWHEELPRESENT 75
#endif

// X button stuff
#if !defined(WM_XBUTTONDOWN)
#define WM_XBUTTONDOWN 0x020B
#define WM_XBUTTONUP 0x020C
#define WM_XBUTTONDBLCLK 0x020D
#define WM_NCXBUTTONDOWN 0x00AB
#define WM_NCXBUTTONUP 0x00AC
#define WM_NCXBUTTONDBLCLK 0x00AD
#define MOUSEEVENTF_XDOWN 0x0080
#define MOUSEEVENTF_XUP 0x0100
#define XBUTTON1 0x0001
#define XBUTTON2 0x0002
#endif


//
// globals
//

#if defined(_MSC_VER)
#pragma comment(linker, "-section:shared,rws")
#pragma data_seg("shared")
#endif
// all data in this shared section *must* be initialized

static HINSTANCE g_hinstance        = NULL;
static DWORD g_processID            = 0;
static EWheelSupport g_wheelSupport = kWheelNone;
static UINT g_wmMouseWheel          = 0;
static DWORD g_threadID             = 0;
static HHOOK g_keyboard             = NULL;
static HHOOK g_mouse                = NULL;
static HHOOK g_getMessage           = NULL;
static HHOOK g_keyboardLL           = NULL;
static HHOOK g_mouseLL              = NULL;
static bool g_screenSaver           = false;
static EHookMode g_mode             = kHOOK_DISABLE;
static UInt32 g_zoneSides           = 0;
static SInt32 g_zoneSize            = 0;
static SInt32 g_xScreen             = 0;
static SInt32 g_yScreen             = 0;
static SInt32 g_wScreen             = 0;
static SInt32 g_hScreen             = 0;
static WPARAM g_deadVirtKey         = 0;
static WPARAM g_deadRelease         = 0;
static LPARAM g_deadLParam          = 0;
static BYTE g_deadKeyState[256]     = {0};
static BYTE g_keyState[256]         = {0};
static DWORD g_hookThread           = 0;
static bool g_fakeInput             = false;

#if defined(_MSC_VER)
#pragma data_seg()
#endif

// keep linker quiet about floating point stuff.  we don't use any
// floating point operations but our includes may define some
// (unused) floating point values.
#ifndef _DEBUG
extern "C" {
int _fltused = 0;
}
#endif

#if !NO_GRAB_KEYBOARD
static WPARAM
makeKeyMsg (UINT virtKey, char c, bool noAltGr) {
    return MAKEWPARAM (MAKEWORD (virtKey & 0xff, (BYTE) c), noAltGr ? 1 : 0);
}

static void
keyboardGetState (BYTE keys[256], DWORD vkCode, bool kf_up) {
    // we have to use GetAsyncKeyState() rather than GetKeyState() because
    // we don't pass through most keys so the event synchronous state
    // doesn't get updated.  we do that because certain modifier keys have
    // side effects, like alt and the windows key.
    if (vkCode < 0 || vkCode >= 256) {
        return;
    }

    // Keep track of key state on our own in case GetAsyncKeyState() fails
    g_keyState[vkCode]   = kf_up ? 0 : 0x80;
    g_keyState[VK_SHIFT] = g_keyState[VK_LSHIFT] | g_keyState[VK_RSHIFT];

    SHORT key;
    // Test whether GetAsyncKeyState() is being honest with us
    key = GetAsyncKeyState (vkCode);

    if (key & 0x80) {
        // The only time we know for sure that GetAsyncKeyState() is working
        // is when it tells us that the current key is down.
        // In this case, update g_keyState to reflect what GetAsyncKeyState()
        // is telling us, just in case we have gotten out of sync

        for (int i = 0; i < 256; ++i) {
            key           = GetAsyncKeyState (i);
            g_keyState[i] = (BYTE) ((key < 0) ? 0x80u : 0);
        }
    }

    // copy g_keyState to keys
    for (int i = 0; i < 256; ++i) {
        keys[i] = g_keyState[i];
    }

    key              = GetKeyState (VK_CAPITAL);
    keys[VK_CAPITAL] = (BYTE) (((key < 0) ? 0x80 : 0) | (key & 1));
}

static bool
doKeyboardHookHandler (WPARAM wParam, LPARAM lParam) {
    DWORD vkCode = static_cast<DWORD> (wParam);
    bool kf_up   = (lParam & (KF_UP << 16)) != 0;

    // check for special events indicating if we should start or stop
    // passing events through and not report them to the server.  this
    // is used to allow the server to synthesize events locally but
    // not pick them up as user events.
    if (wParam == SYNERGY_HOOK_FAKE_INPUT_VIRTUAL_KEY &&
        ((lParam >> 16) & 0xffu) == SYNERGY_HOOK_FAKE_INPUT_SCANCODE) {
        // update flag
        g_fakeInput = ((lParam & 0x80000000u) == 0);
        PostThreadMessage (
            g_threadID, SYNERGY_MSG_DEBUG, 0xff000000u | wParam, lParam);

        // discard event
        return true;
    }

    // if we're expecting fake input then just pass the event through
    // and do not forward to the server
    if (g_fakeInput) {
        PostThreadMessage (
            g_threadID, SYNERGY_MSG_DEBUG, 0xfe000000u | wParam, lParam);
        return false;
    }

    // VK_RSHIFT may be sent with an extended scan code but right shift
    // is not an extended key so we reset that bit.
    if (wParam == VK_RSHIFT) {
        lParam &= ~0x01000000u;
    }

    // tell server about event
    PostThreadMessage (g_threadID, SYNERGY_MSG_DEBUG, wParam, lParam);

    // ignore dead key release
    if ((g_deadVirtKey == wParam || g_deadRelease == wParam) &&
        (lParam & 0x80000000u) != 0) {
        g_deadRelease = 0;
        PostThreadMessage (
            g_threadID, SYNERGY_MSG_DEBUG, wParam | 0x04000000, lParam);
        return false;
    }

    // we need the keyboard state for ToAscii()
    BYTE keys[256];
    keyboardGetState (keys, vkCode, kf_up);

    // ToAscii() maps ctrl+letter to the corresponding control code
    // and ctrl+backspace to delete.  we don't want those translations
    // so clear the control modifier state.  however, if we want to
    // simulate AltGr (which is ctrl+alt) then we must not clear it.
    UINT control = keys[VK_CONTROL] | keys[VK_LCONTROL] | keys[VK_RCONTROL];
    UINT menu    = keys[VK_MENU] | keys[VK_LMENU] | keys[VK_RMENU];
    if ((control & 0x80) == 0 || (menu & 0x80) == 0) {
        keys[VK_LCONTROL] = 0;
        keys[VK_RCONTROL] = 0;
        keys[VK_CONTROL]  = 0;
    } else {
        keys[VK_LCONTROL] = 0x80;
        keys[VK_RCONTROL] = 0x80;
        keys[VK_CONTROL]  = 0x80;
        keys[VK_LMENU]    = 0x80;
        keys[VK_RMENU]    = 0x80;
        keys[VK_MENU]     = 0x80;
    }

    // ToAscii() needs to know if a menu is active for some reason.
    // we don't know and there doesn't appear to be any way to find
    // out.  so we'll just assume a menu is active if the menu key
    // is down.
    // FIXME -- figure out some way to check if a menu is active
    UINT flags = 0;
    if ((menu & 0x80) != 0)
        flags |= 1;

    // if we're on the server screen then just pass numpad keys with alt
    // key down as-is.  we won't pick up the resulting character but the
    // local app will.  if on a client screen then grab keys as usual;
    // if the client is a windows system it'll synthesize the expected
    // character.  if not then it'll probably just do nothing.
    if (g_mode != kHOOK_RELAY_EVENTS) {
        // we don't use virtual keys because we don't know what the
        // state of the numlock key is.  we'll hard code the scan codes
        // instead.  hopefully this works across all keyboards.
        UINT sc = (lParam & 0x01ff0000u) >> 16;
        if (menu &&
            (sc >= 0x47u && sc <= 0x52u && sc != 0x4au && sc != 0x4eu)) {
            return false;
        }
    }

    WORD c = 0;

    // map the key event to a character.  we have to put the dead
    // key back first and this has the side effect of removing it.
    if (g_deadVirtKey != 0) {
        if (ToAscii ((UINT) g_deadVirtKey,
                     (g_deadLParam & 0x10ff0000u) >> 16,
                     g_deadKeyState,
                     &c,
                     flags) == 2) {
            // If ToAscii returned 2, it means that we accidentally removed
            // a double dead key instead of restoring it. Thus, we call
            // ToAscii again with the same parameters to restore the
            // internal dead key state.
            ToAscii ((UINT) g_deadVirtKey,
                     (g_deadLParam & 0x10ff0000u) >> 16,
                     g_deadKeyState,
                     &c,
                     flags);

            // We need to keep track of this because g_deadVirtKey will be
            // cleared later on; this would cause the dead key release to
            // incorrectly restore the dead key state.
            g_deadRelease = g_deadVirtKey;
        }
    }

    UINT scanCode = ((lParam & 0x10ff0000u) >> 16);
    int n         = ToAscii ((UINT) wParam, scanCode, keys, &c, flags);

    // if mapping failed and ctrl and alt are pressed then try again
    // with both not pressed.  this handles the case where ctrl and
    // alt are being used as individual modifiers rather than AltGr.
    // we note that's the case in the message sent back to synergy
    // because there's no simple way to deduce it after the fact.
    // we have to put the dead key back first, if there was one.
    bool noAltGr = false;
    if (n == 0 && (control & 0x80) != 0 && (menu & 0x80) != 0) {
        noAltGr = true;
        PostThreadMessage (
            g_threadID, SYNERGY_MSG_DEBUG, wParam | 0x05000000, lParam);
        if (g_deadVirtKey != 0) {
            if (ToAscii ((UINT) g_deadVirtKey,
                         (g_deadLParam & 0x10ff0000u) >> 16,
                         g_deadKeyState,
                         &c,
                         flags) == 2) {
                ToAscii ((UINT) g_deadVirtKey,
                         (g_deadLParam & 0x10ff0000u) >> 16,
                         g_deadKeyState,
                         &c,
                         flags);
                g_deadRelease = g_deadVirtKey;
            }
        }
        BYTE keys2[256];
        for (size_t i = 0; i < sizeof (keys) / sizeof (keys[0]); ++i) {
            keys2[i] = keys[i];
        }
        keys2[VK_LCONTROL] = 0;
        keys2[VK_RCONTROL] = 0;
        keys2[VK_CONTROL]  = 0;
        keys2[VK_LMENU]    = 0;
        keys2[VK_RMENU]    = 0;
        keys2[VK_MENU]     = 0;
        n = ToAscii ((UINT) wParam, scanCode, keys2, &c, flags);
    }

    PostThreadMessage (g_threadID,
                       SYNERGY_MSG_DEBUG,
                       wParam | ((c & 0xff) << 8) | ((n & 0xff) << 16) |
                           0x06000000,
                       lParam);
    WPARAM charAndVirtKey = 0;
    bool clearDeadKey     = false;
    switch (n) {
        default:
            // key is a dead key

            if (lParam & 0x80000000u)
                // This handles the obscure situation where a key has been
                // pressed which is both a dead key and a normal character
                // depending on which modifiers have been pressed. We
                // break here to prevent it from being considered a dead
                // key.
                break;

            g_deadVirtKey = wParam;
            g_deadLParam  = lParam;
            for (size_t i = 0; i < sizeof (keys) / sizeof (keys[0]); ++i) {
                g_deadKeyState[i] = keys[i];
            }
            break;

        case 0:
            // key doesn't map to a character.  this can happen if
            // non-character keys are pressed after a dead key.
            charAndVirtKey = makeKeyMsg ((UINT) wParam, (char) 0, noAltGr);
            break;

        case 1:
            // key maps to a character composed with dead key
            charAndVirtKey =
                makeKeyMsg ((UINT) wParam, (char) LOBYTE (c), noAltGr);
            clearDeadKey = true;
            break;

        case 2: {
            // previous dead key not composed.  send a fake key press
            // and release for the dead key to our window.
            WPARAM deadCharAndVirtKey =
                makeKeyMsg ((UINT) g_deadVirtKey, (char) LOBYTE (c), noAltGr);
            PostThreadMessage (g_threadID,
                               SYNERGY_MSG_KEY,
                               deadCharAndVirtKey,
                               g_deadLParam & 0x7fffffffu);
            PostThreadMessage (g_threadID,
                               SYNERGY_MSG_KEY,
                               deadCharAndVirtKey,
                               g_deadLParam | 0x80000000u);

            // use uncomposed character
            charAndVirtKey =
                makeKeyMsg ((UINT) wParam, (char) HIBYTE (c), noAltGr);
            clearDeadKey = true;
            break;
        }
    }

    // put back the dead key, if any, for the application to use
    if (g_deadVirtKey != 0) {
        ToAscii ((UINT) g_deadVirtKey,
                 (g_deadLParam & 0x10ff0000u) >> 16,
                 g_deadKeyState,
                 &c,
                 flags);
    }

    // clear out old dead key state
    if (clearDeadKey) {
        g_deadVirtKey = 0;
        g_deadLParam  = 0;
    }

    // forward message to our window.  do this whether or not we're
    // forwarding events to clients because this'll keep our thread's
    // key state table up to date.  that's important for querying
    // the scroll lock toggle state.
    // XXX -- with hot keys for actions we may only need to do this when
    // forwarding.
    if (charAndVirtKey != 0) {
        PostThreadMessage (
            g_threadID, SYNERGY_MSG_DEBUG, charAndVirtKey | 0x07000000, lParam);
        PostThreadMessage (g_threadID, SYNERGY_MSG_KEY, charAndVirtKey, lParam);
    }

    if (g_mode == kHOOK_RELAY_EVENTS) {
        // let certain keys pass through
        switch (wParam) {
            case VK_CAPITAL:
            case VK_NUMLOCK:
            case VK_SCROLL:
                // pass event on.  we want to let these through to
                // the window proc because otherwise the keyboard
                // lights may not stay synchronized.
                break;

            case VK_HANGUL:
                // pass these modifiers if using a low level hook, discard
                // them if not.
                if (g_hookThread == 0) {
                    return true;
                }
                break;

            default:
                // discard
                return true;
        }
    }

    return false;
}

static bool
keyboardHookHandler (WPARAM wParam, LPARAM lParam) {
    return doKeyboardHookHandler (wParam, lParam);
}
#endif

static bool
doMouseHookHandler (WPARAM wParam, SInt32 x, SInt32 y, SInt32 data) {
    switch (wParam) {
        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_XBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
        case WM_MBUTTONDBLCLK:
        case WM_RBUTTONDBLCLK:
        case WM_XBUTTONDBLCLK:
        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP:
        case WM_XBUTTONUP:
        case WM_NCLBUTTONDOWN:
        case WM_NCMBUTTONDOWN:
        case WM_NCRBUTTONDOWN:
        case WM_NCXBUTTONDOWN:
        case WM_NCLBUTTONDBLCLK:
        case WM_NCMBUTTONDBLCLK:
        case WM_NCRBUTTONDBLCLK:
        case WM_NCXBUTTONDBLCLK:
        case WM_NCLBUTTONUP:
        case WM_NCMBUTTONUP:
        case WM_NCRBUTTONUP:
        case WM_NCXBUTTONUP:
            // always relay the event.  eat it if relaying.
            PostThreadMessage (
                g_threadID, SYNERGY_MSG_MOUSE_BUTTON, wParam, data);
            return (g_mode == kHOOK_RELAY_EVENTS);

        case WM_MOUSEWHEEL:
            if (g_mode == kHOOK_RELAY_EVENTS) {
                // relay event
                PostThreadMessage (
                    g_threadID, SYNERGY_MSG_MOUSE_WHEEL, data, 0);
            }
            return (g_mode == kHOOK_RELAY_EVENTS);

        case WM_NCMOUSEMOVE:
        case WM_MOUSEMOVE:
            if (g_mode == kHOOK_RELAY_EVENTS) {
                // relay and eat event
                PostThreadMessage (g_threadID, SYNERGY_MSG_MOUSE_MOVE, x, y);
                return true;
            } else if (g_mode == kHOOK_WATCH_JUMP_ZONE) {
                // low level hooks can report bogus mouse positions that are
                // outside of the screen.  jeez.  naturally we end up getting
                // fake motion in the other direction to get the position back
                // on the screen, which plays havoc with switch on double tap.
                // Server deals with that.  we'll clamp positions onto the
                // screen.  also, if we discard events for positions outside
                // of the screen then the mouse appears to get a bit jerky
                // near the edge.  we can either accept that or pass the bogus
                // events.  we'll try passing the events.
                bool bogus = false;
                if (x < g_xScreen) {
                    x     = g_xScreen;
                    bogus = true;
                } else if (x >= g_xScreen + g_wScreen) {
                    x     = g_xScreen + g_wScreen - 1;
                    bogus = true;
                }
                if (y < g_yScreen) {
                    y     = g_yScreen;
                    bogus = true;
                } else if (y >= g_yScreen + g_hScreen) {
                    y     = g_yScreen + g_hScreen - 1;
                    bogus = true;
                }

                // check for mouse inside jump zone
                bool inside = false;
                if (!inside && (g_zoneSides & kLeftMask) != 0) {
                    inside = (x < g_xScreen + g_zoneSize);
                }
                if (!inside && (g_zoneSides & kRightMask) != 0) {
                    inside = (x >= g_xScreen + g_wScreen - g_zoneSize);
                }
                if (!inside && (g_zoneSides & kTopMask) != 0) {
                    inside = (y < g_yScreen + g_zoneSize);
                }
                if (!inside && (g_zoneSides & kBottomMask) != 0) {
                    inside = (y >= g_yScreen + g_hScreen - g_zoneSize);
                }

                // relay the event
                PostThreadMessage (g_threadID, SYNERGY_MSG_MOUSE_MOVE, x, y);

                // if inside and not bogus then eat the event
                return inside && !bogus;
            }
    }

    // pass the event
    return false;
}

static bool
mouseHookHandler (WPARAM wParam, SInt32 x, SInt32 y, SInt32 data) {
    return doMouseHookHandler (wParam, x, y, data);
}

#if !NO_GRAB_KEYBOARD
static LRESULT CALLBACK
keyboardHook (int code, WPARAM wParam, LPARAM lParam) {
    if (code >= 0) {
        // handle the message
        if (keyboardHookHandler (wParam, lParam)) {
            return 1;
        }
    }

    return CallNextHookEx (g_keyboard, code, wParam, lParam);
}
#endif

static LRESULT CALLBACK
mouseHook (int code, WPARAM wParam, LPARAM lParam) {
    if (code >= 0) {
        // decode message
        const MOUSEHOOKSTRUCT* info = (const MOUSEHOOKSTRUCT*) lParam;
        SInt32 x                    = (SInt32) info->pt.x;
        SInt32 y                    = (SInt32) info->pt.y;
        SInt32 w                    = 0;
        if (wParam == WM_MOUSEWHEEL) {
            // win2k and other systems supporting WM_MOUSEWHEEL in
            // the mouse hook are gratuitously different (and poorly
            // documented).  if a low-level mouse hook is in place
            // it should capture these events so we'll never see
            // them.
            switch (g_wheelSupport) {
                case kWheelModern:
                    w = static_cast<SInt16> (LOWORD (info->dwExtraInfo));
                    break;

                case kWheelWin2000: {
                    const MOUSEHOOKSTRUCTWin2000* info2k =
                        (const MOUSEHOOKSTRUCTWin2000*) lParam;
                    w = static_cast<SInt16> (HIWORD (info2k->mouseData));
                    break;
                }

                default:
                    break;
            }
        }

        // handle the message.  note that we don't handle X buttons
        // here.  that's okay because they're only supported on
        // win2k and winxp and up and on those platforms we'll get
        // get the mouse events through the low level hook.
        if (mouseHookHandler (wParam, x, y, w)) {
            return 1;
        }
    }

    return CallNextHookEx (g_mouse, code, wParam, lParam);
}

static LRESULT CALLBACK
getMessageHook (int code, WPARAM wParam, LPARAM lParam) {
    if (code >= 0) {
        if (g_screenSaver) {
            MSG* msg = reinterpret_cast<MSG*> (lParam);
            if (msg->message == WM_SYSCOMMAND && msg->wParam == SC_SCREENSAVE) {
                // broadcast screen saver started message
                PostThreadMessage (
                    g_threadID, SYNERGY_MSG_SCREEN_SAVER, TRUE, 0);
            }
        }
        if (g_mode == kHOOK_RELAY_EVENTS) {
            MSG* msg = reinterpret_cast<MSG*> (lParam);
            if (g_wheelSupport == kWheelOld && msg->message == g_wmMouseWheel) {
                // post message to our window
                PostThreadMessage (g_threadID,
                                   SYNERGY_MSG_MOUSE_WHEEL,
                                   static_cast<SInt16> (msg->wParam & 0xffffu),
                                   0);

                // zero out the delta in the message so it's (hopefully)
                // ignored
                msg->wParam = 0;
            }
        }
    }

    return CallNextHookEx (g_getMessage, code, wParam, lParam);
}

#if (_WIN32_WINNT >= 0x0400) && defined(_MSC_VER) && !NO_LOWLEVEL_HOOKS

//
// low-level keyboard hook -- this allows us to capture and handle
// alt+tab, alt+esc, ctrl+esc, and windows key hot keys.  on the down
// side, key repeats are not reported to us.
//

#if !NO_GRAB_KEYBOARD
static LRESULT CALLBACK
keyboardLLHook (int code, WPARAM wParam, LPARAM lParam) {
    if (code >= 0) {
        // decode the message
        KBDLLHOOKSTRUCT* info = reinterpret_cast<KBDLLHOOKSTRUCT*> (lParam);
        WPARAM wParam         = info->vkCode;
        LPARAM lParam         = 1;        // repeat code
        lParam |= (info->scanCode << 16); // scan code
        if (info->flags & LLKHF_EXTENDED) {
            lParam |= (1lu << 24); // extended key
        }
        if (info->flags & LLKHF_ALTDOWN) {
            lParam |= (1lu << 29); // context code
        }
        if (info->flags & LLKHF_UP) {
            lParam |= (1lu << 31); // transition
        }
        // FIXME -- bit 30 should be set if key was already down but
        // we don't know that info.  as a result we'll never generate
        // key repeat events.

        // handle the message
        if (keyboardHookHandler (wParam, lParam)) {
            return 1;
        }
    }

    return CallNextHookEx (g_keyboardLL, code, wParam, lParam);
}
#endif

//
// low-level mouse hook -- this allows us to capture and handle mouse
// events very early.  the earlier the better.
//

static LRESULT CALLBACK
mouseLLHook (int code, WPARAM wParam, LPARAM lParam) {
    if (code >= 0) {
        // decode the message
        MSLLHOOKSTRUCT* info = reinterpret_cast<MSLLHOOKSTRUCT*> (lParam);
        SInt32 x             = static_cast<SInt32> (info->pt.x);
        SInt32 y             = static_cast<SInt32> (info->pt.y);
        SInt32 w             = static_cast<SInt16> (HIWORD (info->mouseData));

        // handle the message
        if (mouseHookHandler (wParam, x, y, w)) {
            return 1;
        }
    }

    return CallNextHookEx (g_mouseLL, code, wParam, lParam);
}

#endif

static EWheelSupport
getWheelSupport () {
    // see if modern wheel is present
    if (GetSystemMetrics (SM_MOUSEWHEELPRESENT)) {
        OSVERSIONINFOEX osvi;
        osvi.dwOSVersionInfoSize = sizeof (OSVERSIONINFOEX);
        osvi.dwPlatformId        = VER_PLATFORM_WIN32_NT;
        osvi.dwMajorVersion      = 5;
        osvi.dwMinorVersion      = 0;
        ULONGLONG condMask       = 0;
        VER_SET_CONDITION (condMask, VER_MAJORVERSION, VER_EQUAL);
        VER_SET_CONDITION (condMask, VER_MINORVERSION, VER_EQUAL);
        VER_SET_CONDITION (condMask, VER_PLATFORMID, VER_EQUAL);
        if (VerifyVersionInfo (&osvi,
                               VER_MAJORVERSION | VER_MINORVERSION |
                                   VER_PLATFORMID,
                               condMask)) {
            return kWheelWin2000;
        }
        return kWheelModern;
    }

// not modern.  see if we've got old-style support.
#if defined(MSH_WHEELSUPPORT)
    UINT wheelSupportMsg = RegisterWindowMessage (MSH_WHEELSUPPORT);
    HWND wheelSupportWindow =
        FindWindow (MSH_WHEELMODULE_CLASS, MSH_WHEELMODULE_TITLE);
    if (wheelSupportWindow != NULL && wheelSupportMsg != 0) {
        if (SendMessage (wheelSupportWindow, wheelSupportMsg, 0, 0) != 0) {
            g_wmMouseWheel = RegisterWindowMessage (MSH_MOUSEWHEEL);
            if (g_wmMouseWheel != 0) {
                return kWheelOld;
            }
        }
    }
#endif

    // assume modern.  we don't do anything special in this case
    // except respond to WM_MOUSEWHEEL messages.  GetSystemMetrics()
    // can apparently return FALSE even if a mouse wheel is present
    // though i'm not sure exactly when it does that (WinME returns
    // FALSE for my logitech USB trackball).
    return kWheelModern;
}


//
// external functions
//

BOOL WINAPI
DllMain (HINSTANCE instance, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls (instance);
        if (g_processID == 0) {
            g_hinstance = instance;
            g_processID = GetCurrentProcessId ();
        }
    } else if (reason == DLL_PROCESS_DETACH) {
        if (g_processID == GetCurrentProcessId ()) {
            uninstall ();
            uninstallScreenSaver ();
            g_processID = 0;
            g_hinstance = NULL;
        }
    }
    return TRUE;
}

extern "C" {

// VS2005 hack to not link with the CRT
#if _MSC_VER >= 1400
BOOL WINAPI
_DllMainCRTStartup (HINSTANCE instance, DWORD reason, LPVOID lpreserved) {
    return DllMain (instance, reason, lpreserved);
}

// VS2005 is a bit more bright than VC6 and optimize simple copy loop to
// intrinsic memcpy.
void* __cdecl memcpy (void* _Dst, const void* _Src, size_t _MaxCount) {
    void* _DstBackup = _Dst;
    switch (_MaxCount & 3) {
        case 3:
            ((char*) _Dst)[0] = ((char*) _Src)[0];
            ++(char*&) _Dst;
            ++(char*&) _Src;
            --_MaxCount;
        case 2:
            ((char*) _Dst)[0] = ((char*) _Src)[0];
            ++(char*&) _Dst;
            ++(char*&) _Src;
            --_MaxCount;
        case 1:
            ((char*) _Dst)[0] = ((char*) _Src)[0];
            ++(char*&) _Dst;
            ++(char*&) _Src;
            --_MaxCount;
            break;
        case 0:
            break;

        default:
            __assume (0);
            break;
    }

    // I think it's faster on intel to deference than modify the pointer.
    const size_t max = _MaxCount / sizeof (UINT_PTR);
    for (size_t i = 0; i < max; ++i) {
        ((UINT_PTR*) _Dst)[i] = ((UINT_PTR*) _Src)[i];
    }

    (UINT_PTR*&) _Dst += max;
    (UINT_PTR*&) _Src += max;

    switch (_MaxCount & 3) {
        case 3:
            ((char*) _Dst)[0] = ((char*) _Src)[0];
            ++(char*&) _Dst;
            ++(char*&) _Src;
        case 2:
            ((char*) _Dst)[0] = ((char*) _Src)[0];
            ++(char*&) _Dst;
            ++(char*&) _Src;
        case 1:
            ((char*) _Dst)[0] = ((char*) _Src)[0];
            ++(char*&) _Dst;
            ++(char*&) _Src;
            break;
        case 0:
            break;

        default:
            __assume (0);
            break;
    }

    return _DstBackup;
}
#endif

int
init (DWORD threadID) {
    assert (g_hinstance != NULL);

    // try to open process that last called init() to see if it's
    // still running or if it died without cleaning up.
    if (g_processID != 0 && g_processID != GetCurrentProcessId ()) {
        HANDLE process =
            OpenProcess (STANDARD_RIGHTS_REQUIRED, FALSE, g_processID);
        if (process != NULL) {
            // old process (probably) still exists so refuse to
            // reinitialize this DLL (and thus steal it from the
            // old process).
            int result = CloseHandle (process);
            if (result == false) {
                return 0;
            }
        }

        // clean up after old process.  the system should've already
        // removed the hooks so we just need to reset our state.
        g_hinstance    = GetModuleHandle (_T("synwinhk"));
        g_processID    = GetCurrentProcessId ();
        g_wheelSupport = kWheelNone;
        g_threadID     = 0;
        g_keyboard     = NULL;
        g_mouse        = NULL;
        g_getMessage   = NULL;
        g_keyboardLL   = NULL;
        g_mouseLL      = NULL;
        g_screenSaver  = false;
    }

    // save thread id.  we'll post messages to this thread's
    // message queue.
    g_threadID = threadID;

    // set defaults
    g_mode      = kHOOK_DISABLE;
    g_zoneSides = 0;
    g_zoneSize  = 0;
    g_xScreen   = 0;
    g_yScreen   = 0;
    g_wScreen   = 0;
    g_hScreen   = 0;

    return 1;
}

int
cleanup (void) {
    assert (g_hinstance != NULL);

    if (g_processID == GetCurrentProcessId ()) {
        g_threadID = 0;
    }

    return 1;
}

EHookResult
install () {
    assert (g_hinstance != NULL);
    assert (g_keyboard == NULL);
    assert (g_mouse == NULL);
    assert (g_getMessage == NULL || g_screenSaver);

    // must be initialized
    if (g_threadID == 0) {
        return kHOOK_FAILED;
    }

    // discard old dead keys
    g_deadVirtKey = 0;
    g_deadLParam  = 0;

    // reset fake input flag
    g_fakeInput = false;

    // check for mouse wheel support
    g_wheelSupport = getWheelSupport ();

    // install GetMessage hook (unless already installed)
    if (g_wheelSupport == kWheelOld && g_getMessage == NULL) {
        g_getMessage =
            SetWindowsHookEx (WH_GETMESSAGE, &getMessageHook, g_hinstance, 0);
    }

// install low-level hooks.  we require that they both get installed.
#if (_WIN32_WINNT >= 0x0400) && defined(_MSC_VER) && !NO_LOWLEVEL_HOOKS
    g_mouseLL = SetWindowsHookEx (WH_MOUSE_LL, &mouseLLHook, g_hinstance, 0);
#if !NO_GRAB_KEYBOARD
    g_keyboardLL =
        SetWindowsHookEx (WH_KEYBOARD_LL, &keyboardLLHook, g_hinstance, 0);
    if (g_mouseLL == NULL || g_keyboardLL == NULL) {
        if (g_keyboardLL != NULL) {
            UnhookWindowsHookEx (g_keyboardLL);
            g_keyboardLL = NULL;
        }
        if (g_mouseLL != NULL) {
            UnhookWindowsHookEx (g_mouseLL);
            g_mouseLL = NULL;
        }
    }
#endif
#endif

    // install regular hooks
    if (g_mouseLL == NULL) {
        g_mouse = SetWindowsHookEx (WH_MOUSE, &mouseHook, g_hinstance, 0);
    }
#if !NO_GRAB_KEYBOARD
    if (g_keyboardLL == NULL) {
        g_keyboard =
            SetWindowsHookEx (WH_KEYBOARD, &keyboardHook, g_hinstance, 0);
    }
#endif

    // check that we got all the hooks we wanted
    if ((g_getMessage == NULL && g_wheelSupport == kWheelOld) ||
#if !NO_GRAB_KEYBOARD
        (g_keyboardLL == NULL && g_keyboard == NULL) ||
#endif
        (g_mouseLL == NULL && g_mouse == NULL)) {
        uninstall ();
        return kHOOK_FAILED;
    }

    if (g_keyboardLL != NULL || g_mouseLL != NULL) {
        g_hookThread = GetCurrentThreadId ();
        return kHOOK_OKAY_LL;
    }

    return kHOOK_OKAY;
}

int
uninstall (void) {
    assert (g_hinstance != NULL);

    // discard old dead keys
    g_deadVirtKey = 0;
    g_deadLParam  = 0;

    // uninstall hooks
    if (g_keyboardLL != NULL) {
        UnhookWindowsHookEx (g_keyboardLL);
        g_keyboardLL = NULL;
    }
    if (g_mouseLL != NULL) {
        UnhookWindowsHookEx (g_mouseLL);
        g_mouseLL = NULL;
    }
    if (g_keyboard != NULL) {
        UnhookWindowsHookEx (g_keyboard);
        g_keyboard = NULL;
    }
    if (g_mouse != NULL) {
        UnhookWindowsHookEx (g_mouse);
        g_mouse = NULL;
    }
    if (g_getMessage != NULL && !g_screenSaver) {
        UnhookWindowsHookEx (g_getMessage);
        g_getMessage = NULL;
    }
    g_wheelSupport = kWheelNone;

    return 1;
}

int
installScreenSaver (void) {
    assert (g_hinstance != NULL);

    // must be initialized
    if (g_threadID == 0) {
        return 0;
    }

    // generate screen saver messages
    g_screenSaver = true;

    // install hook unless it's already installed
    if (g_getMessage == NULL) {
        g_getMessage =
            SetWindowsHookEx (WH_GETMESSAGE, &getMessageHook, g_hinstance, 0);
    }

    return (g_getMessage != NULL) ? 1 : 0;
}

int
uninstallScreenSaver (void) {
    assert (g_hinstance != NULL);

    // uninstall hook unless the mouse wheel hook is installed
    if (g_getMessage != NULL && g_wheelSupport != kWheelOld) {
        UnhookWindowsHookEx (g_getMessage);
        g_getMessage = NULL;
    }

    // screen saver hook is no longer installed
    g_screenSaver = false;

    return 1;
}

void
setSides (UInt32 sides) {
    g_zoneSides = sides;
}

void
setZone (SInt32 x, SInt32 y, SInt32 w, SInt32 h, SInt32 jumpZoneSize) {
    g_zoneSize = jumpZoneSize;
    g_xScreen  = x;
    g_yScreen  = y;
    g_wScreen  = w;
    g_hScreen  = h;
}

void
setMode (EHookMode mode) {
    if (mode == g_mode) {
        // no change
        return;
    }
    g_mode = mode;
}
}

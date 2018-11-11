
#pragma once

#include "config.h"

#if X_DISPLAY_MISSING
#	error X11 is required to build barrier
#else
#	include <X11/X.h>
#	include <X11/Xutil.h>
#	define XK_MISCELLANY
#	define XK_XKB_KEYS
#	include <X11/keysymdef.h>
#	if HAVE_X11_EXTENSIONS_DPMS_H
        extern "C" {
#		include <X11/extensions/dpms.h>
        }
#	endif
#	if HAVE_X11_EXTENSIONS_XTEST_H
#		include <X11/extensions/XTest.h>
#	else
#		error The XTest extension is required to build barrier
#	endif
#	if HAVE_X11_EXTENSIONS_XINERAMA_H
        // Xinerama.h may lack extern "C" for inclusion by C++
        extern "C" {
#		include <X11/extensions/Xinerama.h>
        }
#	endif
#	if HAVE_X11_EXTENSIONS_XRANDR_H
#		include <X11/extensions/Xrandr.h>
#	endif
#	if HAVE_XKB_EXTENSION
#		include <X11/XKBlib.h>
#	endif
#	ifdef HAVE_XI2
#		include <X11/extensions/XInput2.h>
#	endif
#endif

class IXWindowsImpl {
public:

    virtual Status XInitThreads() = 0;
    virtual XIOErrorHandler XSetIOErrorHandler(XIOErrorHandler handler) = 0;
    virtual Window do_DefaultRootWindow(Display* display) = 0;
    virtual int XCloseDisplay(Display* display) = 0;
    virtual int XTestGrabControl(Display* display, Bool impervious) = 0;
    virtual void XDestroyIC(XIC ic) = 0;
    virtual Status XCloseIM(XIM im) = 0;
    virtual int XDestroyWindow(Display* display, Window w) = 0;
    virtual int XGetKeyboardControl(Display* display,
                                    XKeyboardState* value_return) = 0;
    virtual int XMoveWindow(Display* display, Window w, int x, int y) = 0;
    virtual int XMapRaised(Display* display, Window w) = 0;
    virtual void XUnsetICFocus(XIC ic) = 0;
    virtual int XUnmapWindow(Display* display, Window w) = 0;
    virtual int XSetInputFocus(Display* display, Window focus, int revert_to,
                               Time time) = 0;
    virtual Bool DPMSQueryExtension(Display* display, int* event_base,
                                    int* error_base) = 0;
    virtual Bool DPMSCapable(Display* display) = 0;
    virtual Status DPMSInfo(Display* display, CARD16* power_level,
                            BOOL* state) = 0;
    virtual Status DPMSForceLevel(Display* display, CARD16 level) =0;
    virtual int XGetInputFocus(Display* display, Window* focus_return,
                               int* revert_to_return) = 0;
    virtual void XSetICFocus(XIC ic) = 0;
    virtual Bool XQueryPointer(Display* display, Window w, Window* root_return,
                               Window* child_return, int* root_x_return,
                               int* root_y_return, int* win_x_return,
                               int* win_y_return, unsigned int* mask_return) =0;
    virtual void XLockDisplay(Display* display) = 0;
    virtual Bool XCheckMaskEvent(Display* display, long event_mask,
                                 XEvent* event_return) = 0;
    virtual XModifierKeymap* XGetModifierMapping(Display* display) = 0;
    virtual int XGrabKey(Display* display, int keycode, unsigned int modifiers,
                         Window grab_window, int owner_events, int pointer_made,
                         int keyboard_mode) = 0;
    virtual int XFreeModifiermap(XModifierKeymap* modmap) = 0;
    virtual int XUngrabKey(Display* display, int keycode,
                           unsigned int modifiers, Window grab_window) = 0;
    virtual int XTestFakeButtonEvent(Display* display, unsigned int button,
                                     int is_press, unsigned long delay) = 0;
    virtual int XFlush(Display* display) = 0;
    virtual int XWarpPointer(Display* display, Window src_w, Window dest_w,
                             int src_x, int src_y,
                             unsigned int src_width, unsigned int src_height,
                             int dest_x, int dest_y) = 0;
    virtual int XTestFakeRelativeMotionEvent(Display* display, int x, int y,
                                             unsigned long delay) = 0;
    virtual KeyCode XKeysymToKeycode(Display* display, KeySym keysym) = 0;
    virtual int XTestFakeKeyEvent(Display* display, unsigned int keycode,
                                  int is_press, unsigned long delay) = 0;
    virtual Display* XOpenDisplay(_Xconst char* display_name) = 0;
    virtual Bool XQueryExtension(Display* display, const char* name,
                                 int* major_opcode_return,
                                 int* first_event_return,
                                 int* first_error_return) = 0;
    virtual Bool XkbLibraryVersion(int* libMajorRtrn, int* libMinorRtrn) = 0;
    virtual Bool XkbQueryExtension(Display* display, int* opcodeReturn,
                                   int* eventBaseReturn, int* errorBaseReturn,
                                   int* majorRtrn, int* minorRtrn) = 0;
    virtual Bool XkbSelectEvents(Display* display, unsigned int deviceID,
                                 unsigned int affect, unsigned int values) = 0;
    virtual Bool XkbSelectEventDetails(Display* display, unsigned int deviceID,
                                       unsigned int eventType,
                                       unsigned long affect,
                                       unsigned long details) = 0;
    virtual Bool XRRQueryExtension(Display* display, int* event_base_return,
                                   int* error_base_return) = 0;
    virtual void XRRSelectInput(Display *display, Window window, int mask) = 0;
    virtual Bool XineramaQueryExtension(Display* display, int* event_base,
                                        int* error_base) = 0;
    virtual Bool XineramaIsActive(Display* display) = 0;
    virtual void* XineramaQueryScreens(Display* display,
                                                     int* number) = 0;
    virtual Window XCreateWindow(Display* display, Window parent, int x, int y,
                                 unsigned int width, unsigned int height,
                                 unsigned int border_width, int depth,
                                 unsigned int klass, Visual* visual,
                                 unsigned long valuemask,
                                 XSetWindowAttributes* attributes) = 0;
    virtual XIM XOpenIM(Display* display, _XrmHashBucketRec* rdb,
                        char* res_name, char* res_class) = 0;
    virtual char* XGetIMValues(XIM im, const char* type, void* ptr) = 0;
    virtual XIC XCreateIC(XIM im, const char* type1, unsigned long data1,
                          const char* type2, unsigned long data2) = 0;
    virtual char* XGetICValues(XIC ic, const char* type,
                               unsigned long* mask) = 0;
    virtual Status XGetWindowAttributes(Display* display, Window w,
                                        XWindowAttributes* attrs) = 0;
    virtual int XSelectInput(Display* display, Window w, long event_mask) = 0;
    virtual Bool XCheckIfEvent(Display* display, XEvent* event,
                               Bool (*predicate)(Display *, XEvent *, XPointer),
                               XPointer arg) = 0;
    virtual Bool XFilterEvent(XEvent* event, Window window) = 0;
    virtual Bool XGetEventData(Display* display,
                               XGenericEventCookie* cookie) = 0;
    virtual void XFreeEventData(Display* display,
                                XGenericEventCookie* cookie) = 0;
    virtual int XDeleteProperty(Display* display, Window w, Atom property) = 0;
    virtual int XResizeWindow(Display* display, Window w, unsigned int width,
                              unsigned int height) = 0;
    virtual int XMaskEvent(Display* display, long event_mask,
                           XEvent* event_return) = 0;
    virtual Status XQueryBestCursor(Display* display, Drawable d,
                                    unsigned int width, unsigned int height,
                                    unsigned int* width_return,
                                    unsigned int* height_return) = 0;
    virtual Pixmap XCreateBitmapFromData(Display* display, Drawable d,
                                         const char* data, unsigned int width,
                                         unsigned int height) = 0;
    virtual Cursor XCreatePixmapCursor(Display* display,
                                       Pixmap source, Pixmap mask,
                                       XColor* foreground_color,
                                       XColor* background_color,
                                       unsigned int x, unsigned int y) = 0;
    virtual int XFreePixmap(Display* display, Pixmap pixmap) = 0;
    virtual Status XQueryTree(Display* display, Window w, Window* root_return,
                              Window* parent_return, Window** children_return,
                              unsigned int* nchildren_return) = 0;
    virtual int XmbLookupString(XIC ic, XKeyPressedEvent* event,
                                char* buffer_return, int bytes_buffer,
                                KeySym* keysym_return, int* status_return) = 0;
    virtual int XLookupString(XKeyEvent* event_struct, char* buffer_return,
                              int bytes_buffer, KeySym* keysym_return,
                              XComposeStatus* status_in_out) = 0;

   virtual Status XSendEvent(Display* display, Window w, Bool propagate,
                             long event_mask, XEvent* event_send) = 0;
    virtual int XSync(Display* display, Bool discard) = 0;
    virtual int XGetPointerMapping(Display* display, unsigned char* map_return,
                                   int nmap) = 0;
    virtual int XGrabKeyboard(Display* display, Window grab_window,
                              Bool owner_events, int pointer_mode,
                              int keyboard_mode, Time time) = 0;
    virtual int XGrabPointer(Display* display, Window grab_window,
                             Bool owner_events, unsigned int event_mask,
                             int  pointer_mode, int keyboard_mode,
                             Window confine_to, Cursor cursor, Time time) = 0;
    virtual int XUngrabKeyboard(Display* display, Time time) = 0;
    virtual int XPending(Display* display) = 0;
    virtual int XPeekEvent(Display* display, XEvent* event_return) = 0;
    virtual Status XkbRefreshKeyboardMapping(XkbMapNotifyEvent* event) = 0;
    virtual int XRefreshKeyboardMapping(XMappingEvent* event_map) = 0;
    virtual int XISelectEvents(Display* display, Window w, XIEventMask* masks,
                               int num_masks) = 0;
    virtual Atom XInternAtom(Display* display, _Xconst char* atom_name,
                             Bool only_if_exists) = 0;
    virtual int XGetScreenSaver(Display* display, int* timeout_return,
                            int*  interval_return, int* prefer_blanking_return,
                            int* allow_exposures_return) = 0;
    virtual int XSetScreenSaver(Display* display, int timeout, int interval,
                                int prefer_blanking, int allow_exposures) = 0;
    virtual int XForceScreenSaver(Display* display, int mode) = 0;
    virtual int XFree(void* data) = 0;
    virtual Status DPMSEnable(Display* display) = 0;
    virtual Status DPMSDisable(Display* display) = 0;
    virtual int XSetSelectionOwner(Display* display, Atom selection, Window w,
                                   Time time) = 0;
    virtual Window XGetSelectionOwner(Display* display, Atom selection) = 0;
    virtual Atom* XListProperties(Display* display, Window w,
                                  int* num_prop_return) = 0;
    virtual char* XGetAtomName(Display* display, Atom atom) = 0;
    virtual void XkbFreeKeyboard(XkbDescPtr xkb, unsigned int which,
                                 Bool freeDesc) = 0;
    virtual XkbDescPtr XkbGetMap(Display* display, unsigned int which,
                                 unsigned int deviceSpec) = 0;
    virtual Status	XkbGetState(Display* display, unsigned int deviceSet,
                                XkbStatePtr rtrnState) = 0;
    virtual int XQueryKeymap(Display* display, char keys_return[32]) = 0;
    virtual Status	XkbGetUpdatedMap(Display* display, unsigned int which,
                                     XkbDescPtr desc) = 0;
    virtual Bool XkbLockGroup(Display* display, unsigned int deviceSpec,
                              unsigned int group) = 0;
    virtual int XDisplayKeycodes(Display* display, int* min_keycodes_return,
                                 int* max_keycodes_return) = 0;
    virtual KeySym* XGetKeyboardMapping(Display* display,
                                        unsigned int first_keycode,
                                        int keycode_count,
                                        int* keysyms_per_keycode_return) = 0;
    virtual int do_XkbKeyNumGroups(XkbDescPtr m_xkb, KeyCode desc) = 0;
    virtual XkbKeyTypePtr do_XkbKeyKeyType(XkbDescPtr m_xkb, KeyCode keycode,
                                           int eGroup) = 0;
    virtual KeySym do_XkbKeySymEntry(XkbDescPtr m_xkb, KeyCode keycode,
                                     int level, int eGroup) = 0;
    virtual Bool do_XkbKeyHasActions(XkbDescPtr m_xkb, KeyCode keycode) = 0;
    virtual XkbAction* do_XkbKeyActionEntry(XkbDescPtr m_xkb, KeyCode keycode,
                                            int level, int eGroup) = 0;
    virtual unsigned char do_XkbKeyGroupInfo(XkbDescPtr m_xkb,
                                             KeyCode keycode) = 0;
    virtual int XNextEvent(Display* display, XEvent* event_return) = 0;
};

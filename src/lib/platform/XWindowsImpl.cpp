
#include "XWindowsImpl.h"

Status XWindowsImpl::XInitThreads()
{
    return ::XInitThreads();
}

XIOErrorHandler XWindowsImpl::XSetIOErrorHandler(XIOErrorHandler handler)
{
    return ::XSetIOErrorHandler(handler);
}

Window XWindowsImpl::do_DefaultRootWindow(Display* display)
{
    return DefaultRootWindow(display);
}

int XWindowsImpl::XCloseDisplay(Display* display)
{
    return ::XCloseDisplay(display);
}

int XWindowsImpl::XTestGrabControl(Display *display, int impervious)
{
    return ::XTestGrabControl(display, impervious);
}

void XWindowsImpl::XDestroyIC(XIC ic)
{
    ::XDestroyIC(ic);
}

Status XWindowsImpl::XCloseIM(XIM im)
{
    return ::XCloseIM(im);
}

int XWindowsImpl::XDestroyWindow(Display* display, Window w)
{
    return ::XDestroyWindow(display, w);
}

int XWindowsImpl::XGetKeyboardControl(Display* display,
                                      XKeyboardState* value_return)
{
    return ::XGetKeyboardControl(display, value_return);
}

int XWindowsImpl::XMoveWindow(Display* display, Window w, int x, int y)
{
    return ::XMoveWindow(display, w, x, y);
}

int XWindowsImpl::XMapRaised(Display* display, Window w)
{
    return ::XMapRaised(display, w);
}

void XWindowsImpl::XUnsetICFocus(XIC ic)
{
    ::XUnsetICFocus(ic);
}

int XWindowsImpl::XUnmapWindow(Display* display, Window w)
{
    return ::XUnmapWindow(display, w);
}

int XWindowsImpl::XSetInputFocus(Display* display, Window focus,
                                 int revert_to, Time time)
{
    return ::XSetInputFocus(display, focus, revert_to, time);
}

Bool XWindowsImpl::DPMSQueryExtension(Display* display, int* event_base,
                                      int* error_base)
{
    return ::DPMSQueryExtension(display, event_base, error_base);
}

Bool XWindowsImpl::DPMSCapable(Display* display)
{
    return ::DPMSCapable(display);
}

Status XWindowsImpl::DPMSInfo(Display* display, CARD16* power_level,
                              BOOL* state)
{
    return ::DPMSInfo(display, power_level, state);
}

Status XWindowsImpl::DPMSForceLevel(Display* display, CARD16 level)
{
    return ::DPMSForceLevel(display,level);
}

int XWindowsImpl::XGetInputFocus(Display* display, Window* focus_return,
                                 int* revert_to_return)
{
    return ::XGetInputFocus(display, focus_return, revert_to_return);
}


void XWindowsImpl::XSetICFocus(XIC ic)
{
    ::XSetICFocus(ic);
}

Bool XWindowsImpl::XQueryPointer(Display* display, Window w,
                                 Window* root_return, Window* child_return,
                                 int* root_x_return, int* root_y_return,
                                 int* win_x_return,  int* win_y_return,
                                 unsigned int* mask_return)
{
    return ::XQueryPointer(display, w, root_return, child_return, root_x_return,
                           root_y_return, win_x_return, win_y_return,
                           mask_return);
}

void XWindowsImpl::XLockDisplay(Display* display)
{
    ::XLockDisplay(display);
}

Bool XWindowsImpl::XCheckMaskEvent(Display* display, long event_mask,
                                   XEvent* event_return)
{
    return ::XCheckMaskEvent(display,event_mask, event_return);
}

XModifierKeymap* XWindowsImpl::XGetModifierMapping(Display* display)
{
    return ::XGetModifierMapping(display);
}

int XWindowsImpl::XGrabKey(Display* display, int keycode,
                           unsigned int modifiers, Window grab_window,
                           int owner_events, int pointer_made,
                           int keyboard_mode)
{
    return ::XGrabKey(display, keycode, modifiers, grab_window, owner_events,
                      pointer_made, keyboard_mode);
}

int XWindowsImpl::XFreeModifiermap(XModifierKeymap* modmap)
{
    return ::XFreeModifiermap(modmap);
}

int XWindowsImpl::XUngrabKey(Display* display, int keycode,
                             unsigned int modifiers, Window grab_window)
{
    return ::XUngrabKey(display, keycode, modifiers, grab_window);
}

int XWindowsImpl::XTestFakeButtonEvent(Display* display, unsigned int button,
                                       int is_press, unsigned long delay)
{
    return ::XTestFakeButtonEvent(display, button, is_press, delay);
}

int XWindowsImpl::XFlush(Display* display)
{
    return ::XFlush(display);
}

int XWindowsImpl::XWarpPointer(Display* display, Window src_w, Window dest_w,
                               int src_x, int src_y,
                               unsigned int src_width, unsigned int src_height,
                               int dest_x, int dest_y)
{
    return ::XWarpPointer(display, src_w, dest_w, src_x, src_y, src_width,
                          src_height, dest_x, dest_y);
}

int XWindowsImpl::XTestFakeRelativeMotionEvent(Display* display, int x, int y,
                                               unsigned long delay)
{
    return ::XTestFakeRelativeMotionEvent(display, x, y, delay);
}

KeyCode XWindowsImpl::XKeysymToKeycode(Display* display, KeySym keysym)
{
    return ::XKeysymToKeycode(display, keysym);
}

int XWindowsImpl::XTestFakeKeyEvent(Display* display, unsigned int keycode,
                                    int is_press, unsigned long delay)
{
    return ::XTestFakeKeyEvent(display, keycode, is_press, delay);
}

Display*  XWindowsImpl::XOpenDisplay(_Xconst char* display_name)
{
    return ::XOpenDisplay(display_name);
}

Bool XWindowsImpl::XQueryExtension(Display* display, const char* name,
                                   int* major_opcode_return,
                                   int* first_event_return,
                                   int* first_error_return)
{
    return ::XQueryExtension(display, name, major_opcode_return,
                             first_event_return, first_error_return);
}

Bool XWindowsImpl::XkbLibraryVersion(int* libMajorRtrn, int* libMinorRtrn)
{
    return ::XkbLibraryVersion(libMajorRtrn, libMinorRtrn);
}

Bool XWindowsImpl::XkbQueryExtension(Display* display, int* opcodeReturn,
                                     int* eventBaseReturn, int* errorBaseReturn,
                                     int* majorRtrn, int* minorRtrn)
{
    return ::XkbQueryExtension(display, opcodeReturn, eventBaseReturn,
                               errorBaseReturn, majorRtrn, minorRtrn);
}


Bool XWindowsImpl::XkbSelectEvents(Display* display, unsigned int deviceID,
                                   unsigned int affect, unsigned int values)
{
    return ::XkbSelectEvents(display, deviceID, affect, values);
}

Bool XWindowsImpl::XkbSelectEventDetails(Display* display,
                                         unsigned int deviceID,
                                         unsigned int eventType,
                                         unsigned long affect,
                                         unsigned long details)
{
    return ::XkbSelectEventDetails(display, deviceID, eventType, affect,
                                   details);
}

Bool XWindowsImpl::XRRQueryExtension(Display* display, int* event_base_return,
                                     int* error_base_return)
{
#if HAVE_X11_EXTENSIONS_XRANDR_H
    return ::XRRQueryExtension(display, event_base_return, error_base_return);
#else
    return false;
#endif
}

void XWindowsImpl::XRRSelectInput(Display *display, Window window, int mask)
{
#if HAVE_X11_EXTENSIONS_XRANDR_H
    ::XRRSelectInput(display, window, mask);
#else
    (void) display; (void) window; (void) mask;
#endif
}

Bool XWindowsImpl::XineramaQueryExtension(Display* display, int* event_base,
                                          int* error_base)
{
#if HAVE_X11_EXTENSIONS_XINERAMA_H
    return ::XineramaQueryExtension(display, event_base, error_base);
#else
    return false;
#endif
}

Bool XWindowsImpl::XineramaIsActive(Display* display)
{
#if HAVE_X11_EXTENSIONS_XINERAMA_H
    return ::XineramaIsActive(display);
#else
    return false;
#endif
}

void* XWindowsImpl::XineramaQueryScreens(Display* display, int* number)
{
#if HAVE_X11_EXTENSIONS_XINERAMA_H
    return ::XineramaQueryScreens(display, number);
#else
    return nullptr;
#endif
}

Window XWindowsImpl::XCreateWindow(Display* display, Window parent,
                                   int x, int y,
                                   unsigned int width, unsigned int height,
                                   unsigned int border_width, int depth,
                                   unsigned int klass, Visual* visual,
                                   unsigned long valuemask,
                                   XSetWindowAttributes* attributes)
{
    return ::XCreateWindow(display, parent, x, y, width, height, border_width,
                           depth, klass, visual, valuemask, attributes);
}

XIM XWindowsImpl::XOpenIM(Display* display, _XrmHashBucketRec* rdb,
                          char* res_name, char* res_class)
{
    return ::XOpenIM(display, rdb, res_name, res_class);
}

char* XWindowsImpl::XGetIMValues(XIM im, const char* type, void* ptr)
{
    return ::XGetIMValues(im, type, ptr, nullptr);
}

XIC XWindowsImpl::XCreateIC(XIM im, const char* type1, unsigned long data,
                            const char* type2, unsigned long data2)
{
    return ::XCreateIC(im, type1, data, type2, data2, nullptr);
}

char* XWindowsImpl::XGetICValues(XIC ic, const char* type, unsigned long* mask)
{
    return ::XGetICValues(ic, type, mask, nullptr);
}

Status XWindowsImpl::XGetWindowAttributes(Display* display, Window w,
                                          XWindowAttributes* attrs)
{
    return ::XGetWindowAttributes(display, w, attrs);
}

int XWindowsImpl::XSelectInput(Display* display, Window w, long event_mask)
{
    return ::XSelectInput(display, w, event_mask);
}

Bool XWindowsImpl::XCheckIfEvent(Display* display, XEvent* event,
                                 Bool (*predicate)(Display *, XEvent *,
                                                   XPointer),
                                 XPointer arg)
{
    return ::XCheckIfEvent(display, event, predicate, arg);
}

Bool XWindowsImpl::XFilterEvent(XEvent* event, Window window)
{
    return ::XFilterEvent(event, window);
}

Bool XWindowsImpl::XGetEventData(Display* display,
                                 XGenericEventCookie* cookie)
{
    return ::XGetEventData(display, cookie);
}

void XWindowsImpl::XFreeEventData(Display* display,
                                  XGenericEventCookie* cookie)
{
    ::XFreeEventData(display, cookie);
}

int XWindowsImpl::XDeleteProperty(Display* display, Window w, Atom property)
{
    return ::XDeleteProperty(display, w, property);
}

int XWindowsImpl::XResizeWindow(Display* display, Window w, unsigned int width,
                                unsigned int height)
{
    return ::XResizeWindow(display, w, width, height);
}

int XWindowsImpl::XMaskEvent(Display* display, long event_mask,
                             XEvent* event_return)
{
    return ::XMaskEvent(display, event_mask, event_return);
}

Status XWindowsImpl::XQueryBestCursor(Display* display, Drawable d,
                                      unsigned int width, unsigned int height,
                                      unsigned int* width_return,
                                      unsigned int* height_return)
{
    return ::XQueryBestCursor(display, d, width, height, width_return,
                              height_return);
}

Pixmap XWindowsImpl::XCreateBitmapFromData(Display* display, Drawable d,
                                           const char* data, unsigned int width,
                                           unsigned int height)
{
    return ::XCreateBitmapFromData(display, d, data, width, height);
}

Cursor XWindowsImpl::XCreatePixmapCursor(Display* display,
                                        Pixmap source, Pixmap mask,
                                        XColor* foreground_color,
                                        XColor* background_color,
                                        unsigned int x, unsigned int y)
{
    return ::XCreatePixmapCursor(display, source, mask, foreground_color,
                                 background_color, x, y);
}

int XWindowsImpl::XFreePixmap(Display* display, Pixmap pixmap)
{
    return ::XFreePixmap(display, pixmap);
}

Status XWindowsImpl::XQueryTree(Display* display, Window w, Window* root_return,
                                Window* parent_return, Window** children_return,
                                unsigned int* nchildren_return)
{
    return ::XQueryTree(display, w, root_return, parent_return, children_return,
                        nchildren_return);
}

int XWindowsImpl::XmbLookupString(XIC ic, XKeyPressedEvent* event,
                                  char* buffer_return, int bytes_buffer,
                                  KeySym* keysym_return, int* status_return)
{
    return ::XmbLookupString(ic, event, buffer_return, bytes_buffer,
                             keysym_return, status_return);
}

int XWindowsImpl::XLookupString(XKeyEvent* event_struct, char* buffer_return,
                                int bytes_buffer, KeySym* keysym_return,
                                XComposeStatus* status_in_out)
{
    return ::XLookupString(event_struct, buffer_return, bytes_buffer,
                           keysym_return, status_in_out);
}

Status XWindowsImpl::XSendEvent(Display* display, Window w, Bool propagate,
                                long event_mask, XEvent* event_send)
{
    return ::XSendEvent(display, w, propagate, event_mask, event_send);
}

int XWindowsImpl::XSync(Display* display, Bool discard)
{
    return ::XSync(display, discard);
}

int XWindowsImpl::XGetPointerMapping(Display* display,
                                     unsigned char* map_return, int nmap)
{
    return ::XGetPointerMapping(display, map_return, nmap);
}

int XWindowsImpl::XGrabKeyboard(Display* display, Window grab_window,
                                Bool owner_events, int pointer_mode,
                                int keyboard_mode, Time time)
{
    return ::XGrabKeyboard(display, grab_window, owner_events, pointer_mode,
                           keyboard_mode, time);
}

int XWindowsImpl::XGrabPointer(Display* display, Window grab_window,
                               Bool owner_events, unsigned int event_mask,
                               int  pointer_mode, int keyboard_mode,
                               Window confine_to, Cursor cursor, Time time)
{
    return ::XGrabPointer(display, grab_window, owner_events, event_mask,
                          pointer_mode, keyboard_mode, confine_to, cursor,
                          time);
}

int XWindowsImpl::XUngrabKeyboard(Display* display, Time time)
{
    return ::XUngrabKeyboard(display, time);
}

int XWindowsImpl::XPending(Display* display)
{
    return ::XPending(display);
}

int XWindowsImpl::XPeekEvent(Display* display, XEvent* event_return)
{
    return ::XPeekEvent(display, event_return);
}

Status XWindowsImpl::XkbRefreshKeyboardMapping(XkbMapNotifyEvent* event)
{
    return ::XkbRefreshKeyboardMapping(event);
}

int XWindowsImpl::XRefreshKeyboardMapping(XMappingEvent* event_map)
{
    return ::XRefreshKeyboardMapping(event_map);
}

int XWindowsImpl::XISelectEvents(Display* display, Window w, XIEventMask* masks,
                                 int num_masks)
{
    return ::XISelectEvents(display, w, masks, num_masks);
}

Atom XWindowsImpl::XInternAtom(Display* display, _Xconst char* atom_name,
                               Bool only_if_exists)
{
    return ::XInternAtom(display, atom_name, only_if_exists);
}

int XWindowsImpl::XGetScreenSaver(Display* display, int* timeout_return,
                                  int*  interval_return,
                                  int* prefer_blanking_return,
                                  int* allow_exposures_return)
{
    return ::XGetScreenSaver(display, timeout_return, interval_return,
                             prefer_blanking_return, allow_exposures_return);
}

int XWindowsImpl::XSetScreenSaver(Display* display, int timeout, int interval,
                                  int prefer_blanking, int allow_exposures)
{
    return ::XSetScreenSaver(display, timeout, interval, prefer_blanking,
                             allow_exposures);
}

int XWindowsImpl::XForceScreenSaver(Display* display, int mode)
{
    return ::XForceScreenSaver(display, mode);
}

int XWindowsImpl::XFree(void* data)
{
    return ::XFree(data);
}

Status XWindowsImpl::DPMSEnable(Display* display)
{
    return ::DPMSEnable(display);
}

Status XWindowsImpl::DPMSDisable(Display* display)
{
    return ::DPMSDisable(display);
}

int XWindowsImpl::XSetSelectionOwner(Display* display, Atom selection, Window w,
                                     Time time)
{
    return ::XSetSelectionOwner(display, selection, w, time);
}

Window XWindowsImpl::XGetSelectionOwner(Display* display, Atom selection)
{
     return ::XGetSelectionOwner(display, selection);
}

Atom* XWindowsImpl::XListProperties(Display* display, Window w,
                                    int* num_prop_return)
{
    return ::XListProperties(display, w, num_prop_return);
}

char* XWindowsImpl::XGetAtomName(Display* display, Atom atom)
{
    return ::XGetAtomName(display, atom);
}

void XWindowsImpl::XkbFreeKeyboard(XkbDescPtr xkb, unsigned int which,
                                   Bool freeDesc)
{
    ::XkbFreeKeyboard(xkb, which, freeDesc);
}

XkbDescPtr XWindowsImpl::XkbGetMap(Display* display, unsigned int which,
                                   unsigned int deviceSpec)
{
    return ::XkbGetMap(display, which, deviceSpec);
}

Status XWindowsImpl::XkbGetState(Display* display, unsigned int deviceSet,
                                 XkbStatePtr rtrnState)
{
    return ::XkbGetState(display, deviceSet, rtrnState);
}

int XWindowsImpl::XQueryKeymap(Display* display, char keys_return[32])
{
    return ::XQueryKeymap(display, keys_return);
}

Status XWindowsImpl::XkbGetUpdatedMap(Display* display, unsigned int which,
                                      XkbDescPtr desc)
{
    return ::XkbGetUpdatedMap(display, which, desc);
}

Bool XWindowsImpl::XkbLockGroup(Display* display, unsigned int deviceSpec,
                                unsigned int group)
{
    return ::XkbLockGroup(display, deviceSpec, group);
}

int XWindowsImpl::XDisplayKeycodes(Display* display, int* min_keycodes_return,
                                   int* max_keycodes_return)
{
    return ::XDisplayKeycodes(display, min_keycodes_return,
                              max_keycodes_return);
}

KeySym* XWindowsImpl::XGetKeyboardMapping(Display* display,
                                          unsigned int first_keycode,
                                          int keycode_count,
                                          int* keysyms_per_keycode_return)
{
    return ::XGetKeyboardMapping(display, first_keycode, keycode_count,
                                 keysyms_per_keycode_return);
}

int XWindowsImpl::do_XkbKeyNumGroups(XkbDescPtr m_xkb, KeyCode desc)

{
    return XkbKeyNumGroups(m_xkb, desc);
}

XkbKeyTypePtr XWindowsImpl::do_XkbKeyKeyType(XkbDescPtr m_xkb, KeyCode keycode,
                                             int eGroup)
{
    return XkbKeyKeyType(m_xkb, keycode, eGroup);
}

KeySym XWindowsImpl::do_XkbKeySymEntry(XkbDescPtr m_xkb, KeyCode keycode,
                                       int level, int eGroup)
{
    return XkbKeySymEntry(m_xkb, keycode, level, eGroup);
}

Bool XWindowsImpl::do_XkbKeyHasActions(XkbDescPtr m_xkb, KeyCode keycode)
{
    return XkbKeyHasActions(m_xkb, keycode);
}

XkbAction* XWindowsImpl::do_XkbKeyActionEntry(XkbDescPtr m_xkb, KeyCode keycode,
                                              int level, int eGroup)
{
    return XkbKeyActionEntry(m_xkb, keycode, level, eGroup);
}

unsigned char XWindowsImpl::do_XkbKeyGroupInfo(XkbDescPtr m_xkb,
                                               KeyCode keycode)
{
    return XkbKeyGroupInfo(m_xkb, keycode);
}

int XWindowsImpl::XNextEvent(Display* display, XEvent* event_return)
{
    return ::XNextEvent(display, event_return);
}

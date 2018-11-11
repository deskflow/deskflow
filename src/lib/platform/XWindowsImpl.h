
#pragma once

#include "IXWindowsImpl.h"

class XWindowsImpl : public IXWindowsImpl {
public:

    virtual Status XInitThreads();
    virtual XIOErrorHandler XSetIOErrorHandler(XIOErrorHandler handler);
    virtual Window do_DefaultRootWindow(Display* display);
    virtual int XCloseDisplay(Display* display);
    virtual int XTestGrabControl(Display* display, Bool impervious);
    virtual void XDestroyIC(XIC ic);
    virtual Status XCloseIM(XIM im);
    virtual int XDestroyWindow(Display* display, Window w);
    virtual int XGetKeyboardControl(Display* display,
                                    XKeyboardState* value_return);
    virtual int XMoveWindow(Display* display, Window w, int x, int y);
    virtual int XMapRaised(Display* display, Window w);
    virtual void XUnsetICFocus(XIC ic);
    virtual int XUnmapWindow(Display* display, Window w);
    virtual int XSetInputFocus(Display* display, Window focus, int revert_to,
                               Time time);
    virtual Bool DPMSQueryExtension(Display* display, int* event_base,
                                    int* error_base);
    virtual Bool DPMSCapable(Display* display);
    virtual Status DPMSInfo(Display* display, CARD16* power_level, BOOL* state);
    virtual Status DPMSForceLevel(Display* display, CARD16 level);
    virtual int XGetInputFocus(Display* display, Window* focus_return,
                               int* revert_to_return);
    virtual void XSetICFocus(XIC ic);
    virtual Bool XQueryPointer(Display* display, Window w, Window* root_return,
                               Window* child_return, int* root_x_return,
                               int* root_y_return, int* win_x_return,
                               int* win_y_return, unsigned int* mask_return);
    virtual void XLockDisplay(Display* display);
    virtual Bool XCheckMaskEvent(Display* display, long event_mask,
                                 XEvent* event_return);
    virtual XModifierKeymap* XGetModifierMapping(Display* display);
    virtual int XGrabKey(Display* display, int keycode, unsigned int modifiers,
                         Window grab_window, int owner_events, int pointer_made,
                         int keyboard_mode);
    virtual int XFreeModifiermap(XModifierKeymap* modmap);
    virtual int XUngrabKey(Display* display, int keycode,
                           unsigned int modifiers, Window grab_window);
    virtual int XTestFakeButtonEvent(Display* display, unsigned int button,
                                     int is_press, unsigned long delay);
    virtual int XFlush(Display* display);
    virtual int XWarpPointer(Display* display, Window src_w, Window dest_w,
                             int src_x, int src_y,
                             unsigned int src_width, unsigned int src_height,
                             int dest_x, int dest_y);
    virtual int XTestFakeRelativeMotionEvent(Display* display, int x, int y,
                                             unsigned long delay);
    virtual KeyCode XKeysymToKeycode(Display* display, KeySym keysym);
    virtual int XTestFakeKeyEvent(Display* display, unsigned int keycode,
                                  int is_press, unsigned long delay);
    virtual Display* XOpenDisplay(_Xconst char* display_name);
    virtual Bool XQueryExtension(Display* display, const char* name,
                                 int* major_opcode_return,
                                 int* first_event_return,
                                 int* first_error_return);
    virtual Bool XkbLibraryVersion(int* libMajorRtrn, int* libMinorRtrn);
    virtual Bool XkbQueryExtension(Display* display, int* opcodeReturn,
                                   int* eventBaseReturn, int* errorBaseReturn,
                                   int* majorRtrn, int* minorRtrn);
    virtual Bool XkbSelectEvents(Display* display, unsigned int deviceID,
                                 unsigned int affect, unsigned int values);
    virtual Bool XkbSelectEventDetails(Display* display, unsigned int deviceID,
                                       unsigned int eventType,
                                       unsigned long affect,
                                       unsigned long details);
    virtual Bool XRRQueryExtension(Display* display, int* event_base_return,
                                   int* error_base_return);
    virtual void XRRSelectInput(Display *display, Window window, int mask);
    virtual Bool XineramaQueryExtension(Display* display, int* event_base,
                                        int* error_base);
    virtual Bool XineramaIsActive(Display* display);
    virtual void* XineramaQueryScreens(Display* display, int* number);
    virtual Window XCreateWindow(Display* display, Window parent, int x, int y,
                                 unsigned int width, unsigned int height,
                                 unsigned int border_width, int depth,
                                 unsigned int klass, Visual* visual,
                                 unsigned long valuemask,
                                 XSetWindowAttributes* attributes);
    virtual XIM XOpenIM(Display* display, _XrmHashBucketRec* rdb,
                        char* res_name, char* res_class);
    virtual char* XGetIMValues(XIM im, const char* type, void* ptr);
    virtual XIC XCreateIC(XIM im, const char* type1, unsigned long data1,
                          const char* type2, unsigned long data2);
    virtual char* XGetICValues(XIC ic, const char* type, unsigned long* mask);
    virtual Status XGetWindowAttributes(Display* display, Window w,
                                        XWindowAttributes* attrs);
    virtual int XSelectInput(Display* display, Window w, long event_mask);
    virtual Bool XCheckIfEvent(Display* display, XEvent* event,
                               Bool (*predicate)(Display *, XEvent *, XPointer),
                               XPointer arg);
    virtual Bool XFilterEvent(XEvent* event, Window window);
    virtual Bool XGetEventData(Display* display,
                               XGenericEventCookie* cookie);
    virtual void XFreeEventData(Display* display,
                                XGenericEventCookie* cookie);
    virtual int XDeleteProperty(Display* display, Window w, Atom property);
    virtual int XResizeWindow(Display* display, Window w, unsigned int width,
                              unsigned int height);
    virtual int XMaskEvent(Display* display, long event_mask,
                           XEvent* event_return);
    virtual Status XQueryBestCursor(Display* display, Drawable d,
                                    unsigned int width, unsigned int height,
                                    unsigned int* width_return,
                                    unsigned int* height_return);
    virtual Pixmap XCreateBitmapFromData(Display* display, Drawable d,
                                         const char* data, unsigned int width,
                                         unsigned int height);
    virtual Cursor XCreatePixmapCursor(Display* display,
                                       Pixmap source, Pixmap mask,
                                       XColor* foreground_color,
                                       XColor* background_color,
                                       unsigned int x, unsigned int y);
    virtual int XFreePixmap(Display* display, Pixmap pixmap);
    virtual Status XQueryTree(Display* display, Window w, Window* root_return,
                              Window* parent_return, Window** children_return,
                              unsigned int* nchildren_return);
    virtual int XmbLookupString(XIC ic, XKeyPressedEvent* event,
                                char* buffer_return, int bytes_buffer,
                                KeySym* keysym_return, int* status_return);
    virtual int XLookupString(XKeyEvent* event_struct, char* buffer_return,
                              int bytes_buffer, KeySym* keysym_return,
                              XComposeStatus* status_in_out);
    virtual Status XSendEvent(Display* display, Window w, Bool propagate,
                              long event_mask, XEvent* event_send);
    virtual int XSync(Display* display, Bool discard);
    virtual int XGetPointerMapping(Display* display, unsigned char* map_return,
                                   int nmap);
    virtual int XGrabKeyboard(Display* display, Window grab_window,
                              Bool owner_events, int pointer_mode,
                              int keyboard_mode, Time time);
    virtual int XGrabPointer(Display* display, Window grab_window,
                             Bool owner_events, unsigned int event_mask,
                             int  pointer_mode, int keyboard_mode,
                             Window confine_to, Cursor cursor, Time time);
    virtual int XUngrabKeyboard(Display* display, Time time);
    virtual int XPending(Display* display);
    virtual int XPeekEvent(Display* display, XEvent* event_return);
    virtual Status XkbRefreshKeyboardMapping(XkbMapNotifyEvent* event);
    virtual int XRefreshKeyboardMapping(XMappingEvent* event_map);
    virtual int XISelectEvents(Display* display, Window w, XIEventMask* masks,
                               int num_masks);
    virtual Atom XInternAtom(Display* display, _Xconst char* atom_name,
                             Bool only_if_exists);
    virtual int XGetScreenSaver(Display* display, int* timeout_return,
                                int*  interval_return,
                                int* prefer_blanking_return,
                                int* allow_exposures_return);
    virtual int XSetScreenSaver(Display* display, int timeout, int interval,
                                int prefer_blanking, int allow_exposures);
    virtual int XForceScreenSaver(Display* display, int mode);
    virtual int XFree(void* data);
    virtual Status DPMSEnable(Display* display);
    virtual Status DPMSDisable(Display* display);
    virtual int XSetSelectionOwner(Display* display, Atom selection, Window w,
                                   Time time);
    virtual Window XGetSelectionOwner(Display* display, Atom selection);
    virtual Atom* XListProperties(Display* display, Window w,
                                  int* num_prop_return);
    virtual char* XGetAtomName(Display* display, Atom atom);
    virtual void XkbFreeKeyboard(XkbDescPtr xkb, unsigned int which,
                                 Bool freeDesc);
    virtual XkbDescPtr XkbGetMap(Display* display, unsigned int which,
                                 unsigned int deviceSpec);
    virtual Status	XkbGetState(Display* display, unsigned int deviceSet,
                                XkbStatePtr rtrnState);
    virtual int XQueryKeymap(Display* display, char keys_return[32]);
    virtual Status	XkbGetUpdatedMap(Display* display, unsigned int which,
                                     XkbDescPtr desc);
    virtual Bool XkbLockGroup(Display* display, unsigned int deviceSpec,
                              unsigned int group);
    virtual int XDisplayKeycodes(Display* display, int* min_keycodes_return,
                                 int* max_keycodes_return);
    virtual KeySym* XGetKeyboardMapping(Display* display,
                                        unsigned int first_keycode,
                                        int keycode_count,
                                        int* keysyms_per_keycode_return);
    virtual int do_XkbKeyNumGroups(XkbDescPtr m_xkb, KeyCode desc);
    virtual XkbKeyTypePtr do_XkbKeyKeyType(XkbDescPtr m_xkb, KeyCode keycode,
                                           int eGroup);
    virtual KeySym do_XkbKeySymEntry(XkbDescPtr m_xkb, KeyCode keycode,
                                     int level, int eGroup);
    virtual Bool do_XkbKeyHasActions(XkbDescPtr m_xkb, KeyCode keycode);
    virtual XkbAction* do_XkbKeyActionEntry(XkbDescPtr m_xkb, KeyCode keycode,
                                            int level, int eGroup);
    virtual unsigned char do_XkbKeyGroupInfo(XkbDescPtr m_xkb,
                                             KeyCode keycode);
    virtual int XNextEvent(Display* display, XEvent* event_return);
};

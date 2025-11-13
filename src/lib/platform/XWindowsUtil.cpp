/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "base/Log.h" //Include First

#include "base/String.h"
#include "platform/XWindowsUtil.h"

#include <X11/Xatom.h>

//
// XWindowsUtil
//

bool XWindowsUtil::getWindowProperty(
    Display *display, Window window, Atom property, std::string *data, Atom *type, int32_t *format, bool deleteProperty
)
{
  assert(display != nullptr);

  Atom actualType;
  int actualDatumSize;

  // ignore errors.  XGetWindowProperty() will report failure.
  XWindowsUtil::ErrorLock lock(display);

  // read the property
  bool okay = true;
  const long length = XMaxRequestSize(display);
  long offset = 0;
  unsigned long bytesLeft = 1;
  while (bytesLeft != 0) {
    // get more data
    unsigned long numItems;
    unsigned char *rawData;
    if (XGetWindowProperty(
            display, window, property, offset, length, False, AnyPropertyType, &actualType, &actualDatumSize, &numItems,
            &bytesLeft, &rawData
        ) != Success ||
        actualType == None || actualDatumSize == 0) {
      // failed
      okay = false;
      break;
    }

    // compute bytes read and advance offset
    unsigned long numBytes;
    switch (actualDatumSize) {
    default:
    case 8:
      numBytes = numItems;
      offset += numItems / 4;
      break;

    case 16:
      numBytes = 2 * numItems;
      offset += numItems / 2;
      break;

    case 32:
      numBytes = 4 * numItems;
      offset += numItems;
      break;
    }

    // append data
    if (data != nullptr) {
      data->append((char *)rawData, numBytes);
    } else {
      // data is not required so don't try to get any more
      bytesLeft = 0;
    }

    // done with returned data
    XFree(rawData);
  }

  // delete the property if requested
  if (deleteProperty) {
    XDeleteProperty(display, window, property);
  }

  // save property info
  if (type != nullptr) {
    *type = actualType;
  }
  if (format != nullptr) {
    *format = actualDatumSize;
  }

  if (okay) {
    LOG(
        (CLOG_DEBUG2 "read property %d on window 0x%08x: bytes=%d", property, window,
         (data == nullptr) ? 0 : data->size())
    );
    return true;
  } else {
    LOG_DEBUG2("can't read property %d on window 0x%08x", property, window);
    return false;
  }
}

bool XWindowsUtil::setWindowProperty(
    Display *display, Window window, Atom property, const void *vdata, uint32_t size, Atom type, int32_t format
)
{
  const uint32_t length = 4 * XMaxRequestSize(display);
  const auto *data = static_cast<const unsigned char *>(vdata);
  auto datumSize = static_cast<uint32_t>(format / 8);
  // format 32 on 64bit systems is 8 bytes not 4.
  if (format == 32) {
    datumSize = sizeof(Atom);
  }

  // save errors
  bool error = false;
  XWindowsUtil::ErrorLock lock(display, &error);

  // how much data to send in first chunk?
  uint32_t chunkSize = size;
  if (chunkSize > length) {
    chunkSize = length;
  }

  // send first chunk
  XChangeProperty(display, window, property, type, format, PropModeReplace, data, chunkSize / datumSize);

  // append remaining chunks
  data += chunkSize;
  size -= chunkSize;
  while (!error && size > 0) {
    chunkSize = size;
    if (chunkSize > length) {
      chunkSize = length;
    }
    XChangeProperty(display, window, property, type, format, PropModeAppend, data, chunkSize / datumSize);
    data += chunkSize;
    size -= chunkSize;
  }

  return !error;
}

Time XWindowsUtil::getCurrentTime(Display *display, Window window)
{
  XLockDisplay(display);
  // select property events on window
  XWindowAttributes attr;
  XGetWindowAttributes(display, window, &attr);
  XSelectInput(display, window, attr.your_event_mask | PropertyChangeMask);

  // make a property name to receive dummy change
  Atom atom = XInternAtom(display, "TIMESTAMP", False);

  // do a zero-length append to get the current time
  unsigned char dummy;
  XChangeProperty(display, window, atom, XA_INTEGER, 8, PropModeAppend, &dummy, 0);

  // look for property notify events with the following
  PropertyNotifyPredicateInfo filter;
  filter.m_window = window;
  filter.m_property = atom;

  // wait for reply
  XEvent xevent;
  XIfEvent(display, &xevent, &XWindowsUtil::propertyNotifyPredicate, (XPointer)&filter);
  assert(xevent.type == PropertyNotify);
  assert(xevent.xproperty.window == window);
  assert(xevent.xproperty.atom == atom);

  // restore event mask
  XSelectInput(display, window, attr.your_event_mask);
  XUnlockDisplay(display);

  return xevent.xproperty.time;
}

std::string XWindowsUtil::atomToString(Display *display, Atom atom)
{
  if (atom == 0) {
    return "None";
  }

  bool error = false;
  XWindowsUtil::ErrorLock lock(display, &error);
  char *name = XGetAtomName(display, atom);
  if (error) {
    return deskflow::string::sprintf("<UNKNOWN> (%d)", (int)atom);
  } else {
    std::string msg = deskflow::string::sprintf("%s (%d)", name, (int)atom);
    XFree(name);
    return msg;
  }
}

std::string XWindowsUtil::atomsToString(Display *display, const Atom *atom, uint32_t num)
{
  auto **names = new char *[num];
  bool error = false;
  XWindowsUtil::ErrorLock lock(display, &error);
  XGetAtomNames(display, const_cast<Atom *>(atom), (int)num, names);
  std::string msg;
  if (error) {
    for (uint32_t i = 0; i < num; ++i) {
      msg += deskflow::string::sprintf("<UNKNOWN> (%d), ", (int)atom[i]);
    }
  } else {
    for (uint32_t i = 0; i < num; ++i) {
      msg += deskflow::string::sprintf("%s (%d), ", names[i], (int)atom[i]);
      XFree(names[i]);
    }
  }
  delete[] names;
  if (msg.size() > 2) {
    msg.erase(msg.size() - 2);
  }
  return msg;
}

void XWindowsUtil::convertAtomProperty(std::string &data)
{
  // as best i can tell, 64-bit systems don't pack Atoms into properties
  // as 32-bit numbers but rather as the 64-bit numbers they are.  that
  // seems wrong but we have to cope.  sometimes we'll get a list of
  // atoms that's 8*n+4 bytes long, missing the trailing 4 bytes which
  // should all be 0.  since we're going to reference the Atoms as
  // 64-bit numbers we have to ensure the last number is a full 64 bits.
  if (sizeof(Atom) != 4 && ((data.size() / 4) & 1) != 0) {
    uint32_t zero = 0;
    data.append(reinterpret_cast<char *>(&zero), sizeof(zero));
  }
}

void XWindowsUtil::appendAtomData(std::string &data, Atom atom)
{
  data.append(reinterpret_cast<char *>(&atom), sizeof(Atom));
}

void XWindowsUtil::replaceAtomData(std::string &data, uint32_t index, Atom atom)
{
  data.replace(index * sizeof(Atom), sizeof(Atom), reinterpret_cast<const char *>(&atom), sizeof(Atom));
}

void XWindowsUtil::appendTimeData(std::string &data, Time time)
{
  data.append(reinterpret_cast<char *>(&time), sizeof(Time));
}

Bool XWindowsUtil::propertyNotifyPredicate(Display *, XEvent *xevent, XPointer arg)
{
  const auto *filter = reinterpret_cast<PropertyNotifyPredicateInfo *>(arg);
  return (xevent->type == PropertyNotify && xevent->xproperty.window == filter->m_window &&
          xevent->xproperty.atom == filter->m_property && xevent->xproperty.state == PropertyNewValue)
             ? True
             : False;
}

//
// XWindowsUtil::ErrorLock
//

XWindowsUtil::ErrorLock *XWindowsUtil::ErrorLock::s_top = nullptr;

XWindowsUtil::ErrorLock::ErrorLock(Display *display) : m_display(display)
{
  install(&XWindowsUtil::ErrorLock::ignoreHandler, nullptr);
}

XWindowsUtil::ErrorLock::ErrorLock(Display *display, bool *flag) : m_display(display)
{
  install(&XWindowsUtil::ErrorLock::saveHandler, flag);
}

XWindowsUtil::ErrorLock::ErrorLock(Display *display, ErrorHandler handler, void *data) : m_display(display)
{
  install(handler, data);
}

XWindowsUtil::ErrorLock::~ErrorLock()
{
  // make sure everything finishes before uninstalling handler
  if (m_display != nullptr) {
    XSync(m_display, False);
  }

  // restore old handler
  XSetErrorHandler(m_oldXHandler);
  s_top = m_next;
}

void XWindowsUtil::ErrorLock::install(ErrorHandler handler, void *data)
{
  // make sure everything finishes before installing handler
  if (m_display != nullptr) {
    XSync(m_display, False);
  }

  // install handler
  m_handler = handler;
  m_userData = data;
  m_oldXHandler = XSetErrorHandler(&XWindowsUtil::ErrorLock::internalHandler);
  m_next = s_top;
  s_top = this;
}

int XWindowsUtil::ErrorLock::internalHandler(Display *display, XErrorEvent *event)
{
  if (s_top != nullptr && s_top->m_handler != nullptr) {
    s_top->m_handler(display, event, s_top->m_userData);
  }
  return 0;
}

void XWindowsUtil::ErrorLock::ignoreHandler(Display *, XErrorEvent *e, void *)
{
  LOG_DEBUG1("ignoring X error: %d", e->error_code);
}

void XWindowsUtil::ErrorLock::saveHandler(Display *display, XErrorEvent *e, void *flag)
{
  char errtxt[1024];
  XGetErrorText(display, e->error_code, errtxt, 1023);
  LOG_DEBUG1("flagging X error: %d - %.1023s", e->error_code, errtxt);
  *static_cast<bool *>(flag) = true;
}

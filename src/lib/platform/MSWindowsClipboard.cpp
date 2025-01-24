/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/MSWindowsClipboard.h"

#include "arch/win32/ArchMiscWindows.h"
#include "base/Log.h"
#include "platform/MSWindowsClipboardBitmapConverter.h"
#include "platform/MSWindowsClipboardFacade.h"
#include "platform/MSWindowsClipboardHTMLConverter.h"
#include "platform/MSWindowsClipboardTextConverter.h"
#include "platform/MSWindowsClipboardUTF16Converter.h"

//
// MSWindowsClipboard
//

UINT MSWindowsClipboard::s_ownershipFormat = 0;

MSWindowsClipboard::MSWindowsClipboard(HWND window)
    : m_window(window),
      m_time(0),
      m_facade(new MSWindowsClipboardFacade()),
      m_deleteFacade(true)
{
  // add converters, most desired first
  m_converters.push_back(new MSWindowsClipboardUTF16Converter);
  m_converters.push_back(new MSWindowsClipboardBitmapConverter);
  m_converters.push_back(new MSWindowsClipboardHTMLConverter);
}

MSWindowsClipboard::~MSWindowsClipboard()
{
  clearConverters();

  // dependency injection causes confusion over ownership, so we need
  // logic to decide whether or not we delete the facade. there must
  // be a more elegant way of doing this.
  if (m_deleteFacade)
    delete m_facade;
}

void MSWindowsClipboard::setFacade(IMSWindowsClipboardFacade &facade)
{
  delete m_facade;
  m_facade = &facade;
  m_deleteFacade = false;
}

bool MSWindowsClipboard::emptyUnowned()
{
  LOG((CLOG_DEBUG "empty clipboard"));

  // empty the clipboard (and take ownership)
  if (!EmptyClipboard()) {
    // unable to cause this in integ tests, but this error has never
    // actually been reported by users.
    LOG((CLOG_WARN "failed to grab clipboard"));
    return false;
  }

  return true;
}

bool MSWindowsClipboard::empty()
{
  if (!emptyUnowned()) {
    return false;
  }

  // mark clipboard as being owned by deskflow
  HGLOBAL data = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, 1);
  if (NULL == SetClipboardData(getOwnershipFormat(), data)) {
    LOG((CLOG_WARN "failed to set clipboard data"));
    GlobalFree(data);
    return false;
  }

  return true;
}

void MSWindowsClipboard::add(EFormat format, const std::string &data)
{
  // exit early if there is no data to prevent spurious "failed to convert clipboard data" errors
  if (data.empty()) {
    LOG((CLOG_DEBUG "not adding 0 bytes to clipboard format: %d", format));
    return;
  }
  bool isSucceeded = false;
  // convert data to win32 form
  for (ConverterList::const_iterator index = m_converters.begin(); index != m_converters.end(); ++index) {
    IMSWindowsClipboardConverter *converter = *index;

    // skip converters for other formats
    if (converter->getFormat() == format) {
      HANDLE win32Data = converter->fromIClipboard(data);
      if (win32Data != NULL) {
        LOG((CLOG_DEBUG "add %d bytes to clipboard format: %d", data.size(), format));
        m_facade->write(win32Data, converter->getWin32Format());
        isSucceeded = true;
        break;
      } else {
        LOG((CLOG_DEBUG "failed to convert clipboard data to platform format"));
      }
    }
  }

  if (!isSucceeded) {
    LOG((CLOG_DEBUG "missed clipboard data convert for format: %d", format));
  }
}

bool MSWindowsClipboard::open(Time time) const
{
  LOG((CLOG_DEBUG "open clipboard"));

  if (!OpenClipboard(m_window)) {
    LOG((CLOG_WARN "failed to open clipboard: %d", GetLastError()));
    return false;
  }

  m_time = time;

  return true;
}

void MSWindowsClipboard::close() const
{
  LOG((CLOG_DEBUG "close clipboard"));
  CloseClipboard();
}

IClipboard::Time MSWindowsClipboard::getTime() const
{
  return m_time;
}

bool MSWindowsClipboard::has(EFormat format) const
{
  for (ConverterList::const_iterator index = m_converters.begin(); index != m_converters.end(); ++index) {
    IMSWindowsClipboardConverter *converter = *index;
    if (converter->getFormat() == format) {
      if (IsClipboardFormatAvailable(converter->getWin32Format())) {
        return true;
      }
    }
  }
  return false;
}

std::string MSWindowsClipboard::get(EFormat format) const
{
  // find the converter for the first clipboard format we can handle
  IMSWindowsClipboardConverter *converter = NULL;
  for (ConverterList::const_iterator index = m_converters.begin(); index != m_converters.end(); ++index) {

    converter = *index;
    if (converter->getFormat() == format) {
      break;
    }
    converter = NULL;
  }

  // if no converter then we don't recognize any formats
  if (converter == NULL) {
    LOG((CLOG_WARN "no converter for format %d", format));
    return std::string();
  }

  // get a handle to the clipboard data
  HANDLE win32Data = GetClipboardData(converter->getWin32Format());
  if (win32Data == NULL) {
    // nb: can't cause this using integ tests; this is only caused when
    // the selected converter returns an invalid format -- which you
    // cannot cause using public functions.
    return std::string();
  }

  // convert
  return converter->toIClipboard(win32Data);
}

void MSWindowsClipboard::clearConverters()
{
  for (ConverterList::iterator index = m_converters.begin(); index != m_converters.end(); ++index) {
    delete *index;
  }
  m_converters.clear();
}

bool MSWindowsClipboard::isOwnedByDeskflow()
{
  // create ownership format if we haven't yet
  if (s_ownershipFormat == 0) {
    s_ownershipFormat = RegisterClipboardFormat(TEXT("Deskflow Ownership"));
  }
  return (IsClipboardFormatAvailable(getOwnershipFormat()) != 0);
}

UINT MSWindowsClipboard::getOwnershipFormat()
{
  // create ownership format if we haven't yet
  if (s_ownershipFormat == 0) {
    s_ownershipFormat = RegisterClipboardFormat(TEXT("Deskflow Ownership"));
  }

  // return the format
  return s_ownershipFormat;
}

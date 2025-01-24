/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/OSXClipboard.h"

#include "arch/XArch.h"
#include "base/Log.h"
#include "deskflow/Clipboard.h"
#include "platform/OSXClipboardBMPConverter.h"
#include "platform/OSXClipboardHTMLConverter.h"
#include "platform/OSXClipboardTextConverter.h"
#include "platform/OSXClipboardUTF16Converter.h"
#include "platform/OSXClipboardUTF8Converter.h"

//
// OSXClipboard
//

OSXClipboard::OSXClipboard() : m_time(0), m_pboard(NULL)
{
  m_converters.push_back(new OSXClipboardHTMLConverter);
  m_converters.push_back(new OSXClipboardBMPConverter);
  m_converters.push_back(new OSXClipboardUTF8Converter);
  m_converters.push_back(new OSXClipboardUTF16Converter);
  m_converters.push_back(new OSXClipboardTextConverter);

  OSStatus createErr = PasteboardCreate(kPasteboardClipboard, &m_pboard);
  if (createErr != noErr) {
    LOG((CLOG_WARN "failed to create clipboard reference: error %i", createErr));
    LOG((CLOG_ERR "unable to connect to pasteboard, clipboard sharing disabled", createErr));
    m_pboard = NULL;
    return;
  }

  OSStatus syncErr = PasteboardSynchronize(m_pboard);
  if (syncErr != noErr) {
    LOG((CLOG_WARN "failed to syncronize clipboard: error %i", syncErr));
  }
}

OSXClipboard::~OSXClipboard()
{
  clearConverters();
}

bool OSXClipboard::empty()
{
  LOG((CLOG_DEBUG "emptying clipboard"));
  if (m_pboard == NULL)
    return false;

  OSStatus err = PasteboardClear(m_pboard);
  if (err != noErr) {
    LOG((CLOG_WARN "failed to clear clipboard: error %i", err));
    return false;
  }

  return true;
}

bool OSXClipboard::synchronize()
{
  if (m_pboard == NULL)
    return false;

  PasteboardSyncFlags flags = PasteboardSynchronize(m_pboard);
  LOG((CLOG_DEBUG2 "flags: %x", flags));

  if (flags & kPasteboardModified) {
    return true;
  }
  return false;
}

void OSXClipboard::add(EFormat format, const std::string &data)
{
  if (m_pboard == NULL)
    return;

  LOG((CLOG_DEBUG "add %d bytes to clipboard format: %d", data.size(), format));
  if (format == IClipboard::kText) {
    LOG((CLOG_DEBUG "format of data to be added to clipboard was kText"));
  } else if (format == IClipboard::kBitmap) {
    LOG((CLOG_DEBUG "format of data to be added to clipboard was kBitmap"));
  } else if (format == IClipboard::kHTML) {
    LOG((CLOG_DEBUG "format of data to be added to clipboard was kHTML"));
  }

  for (ConverterList::const_iterator index = m_converters.begin(); index != m_converters.end(); ++index) {

    IOSXClipboardConverter *converter = *index;

    // skip converters for other formats
    if (converter->getFormat() == format) {
      std::string osXData = converter->fromIClipboard(data);
      CFStringRef flavorType = converter->getOSXFormat();
      CFDataRef dataRef = CFDataCreate(kCFAllocatorDefault, (uint8_t *)osXData.data(), osXData.size());
      PasteboardItemID itemID = 0;

      if (dataRef) {
        PasteboardPutItemFlavor(m_pboard, itemID, flavorType, dataRef, kPasteboardFlavorNoFlags);

        CFRelease(dataRef);
        LOG((CLOG_DEBUG "added %d bytes to clipboard format: %d", data.size(), format));
      }
    }
  }
}

bool OSXClipboard::open(Time time) const
{
  if (m_pboard == NULL)
    return false;

  LOG((CLOG_DEBUG "opening clipboard"));
  m_time = time;
  return true;
}

void OSXClipboard::close() const
{
  LOG((CLOG_DEBUG "closing clipboard"));
  /* not needed */
}

IClipboard::Time OSXClipboard::getTime() const
{
  return m_time;
}

bool OSXClipboard::has(EFormat format) const
{
  if (m_pboard == NULL)
    return false;

  PasteboardItemID item;
  PasteboardGetItemIdentifier(m_pboard, (CFIndex)1, &item);

  for (ConverterList::const_iterator index = m_converters.begin(); index != m_converters.end(); ++index) {
    IOSXClipboardConverter *converter = *index;
    if (converter->getFormat() == format) {
      PasteboardFlavorFlags flags;
      CFStringRef type = converter->getOSXFormat();

      OSStatus res;

      if ((res = PasteboardGetItemFlavorFlags(m_pboard, item, type, &flags)) == noErr) {
        return true;
      }
    }
  }

  return false;
}

std::string OSXClipboard::get(EFormat format) const
{
  CFStringRef type;
  PasteboardItemID item;
  std::string result;

  if (m_pboard == NULL)
    return result;

  PasteboardGetItemIdentifier(m_pboard, (CFIndex)1, &item);

  // find the converter for the first clipboard format we can handle
  IOSXClipboardConverter *converter = NULL;
  for (ConverterList::const_iterator index = m_converters.begin(); index != m_converters.end(); ++index) {
    converter = *index;

    PasteboardFlavorFlags flags;
    type = converter->getOSXFormat();

    if (converter->getFormat() == format && PasteboardGetItemFlavorFlags(m_pboard, item, type, &flags) == noErr) {
      break;
    }
    converter = NULL;
  }

  // if no converter then we don't recognize any formats
  if (converter == NULL) {
    LOG((CLOG_DEBUG "unable to find converter for data"));
    return result;
  }

  // get the clipboard data.
  CFDataRef buffer = NULL;
  try {
    OSStatus err = PasteboardCopyItemFlavorData(m_pboard, item, type, &buffer);

    if (err != noErr) {
      throw err;
    }

    result = std::string((char *)CFDataGetBytePtr(buffer), CFDataGetLength(buffer));
  } catch (OSStatus err) {
    LOG((CLOG_DEBUG "exception thrown in OSXClipboard::get MacError (%d)", err));
  } catch (...) {
    LOG((CLOG_DEBUG "unknown exception in OSXClipboard::get"));
    RETHROW_XTHREAD
  }

  if (buffer != NULL)
    CFRelease(buffer);

  return converter->toIClipboard(result);
}

void OSXClipboard::clearConverters()
{
  if (m_pboard == NULL)
    return;

  for (ConverterList::iterator index = m_converters.begin(); index != m_converters.end(); ++index) {
    delete *index;
  }
  m_converters.clear();
}

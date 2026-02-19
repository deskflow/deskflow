/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/OSXClipboard.h"

#include "arch/ArchException.h"
#include "base/Log.h"
#include "platform/OSXClipboardBMPConverter.h"
#include "platform/OSXClipboardHTMLConverter.h"
#include "platform/OSXClipboardImageConverter.h"
#include "platform/OSXClipboardTextConverter.h"
#include "platform/OSXClipboardUTF16Converter.h"
#include "platform/OSXClipboardUTF8Converter.h"

#include <chrono>
#include <thread>

namespace {

bool readPasteboardFlavorData(PasteboardRef pasteboard, PasteboardItemID item, CFStringRef flavorType, std::string &out)
{
  CFDataRef buffer = nullptr;
  OSStatus err = PasteboardCopyItemFlavorData(pasteboard, item, flavorType, &buffer);
  if (err != noErr || buffer == nullptr) {
    if (buffer != nullptr) {
      CFRelease(buffer);
    }
    return false;
  }

  out.assign(reinterpret_cast<const char *>(CFDataGetBytePtr(buffer)), static_cast<size_t>(CFDataGetLength(buffer)));
  CFRelease(buffer);
  return true;
}

} // namespace

//
// OSXClipboard
//

OSXClipboard::OSXClipboard() : m_time(0), m_pboard(nullptr)
{
  m_converters.push_back(new OSXClipboardHTMLConverter);
  m_converters.push_back(new OSXClipboardImageConverter(CFSTR("public.tiff"), "TIFF"));
  m_converters.push_back(new OSXClipboardImageConverter(CFSTR("public.png"), "PNG"));
  m_converters.push_back(new OSXClipboardImageConverter(CFSTR("NeXT TIFF v4.0 pasteboard type"), "TIFF"));
  m_converters.push_back(new OSXClipboardBMPConverter);
  m_converters.push_back(new OSXClipboardUTF8Converter);
  m_converters.push_back(new OSXClipboardUTF16Converter);
  m_converters.push_back(new OSXClipboardTextConverter);

  OSStatus createErr = PasteboardCreate(kPasteboardClipboard, &m_pboard);
  if (createErr != noErr) {
    LOG_WARN("failed to create clipboard reference: error %i", createErr);
    LOG_ERR("unable to connect to pasteboard, clipboard sharing disabled", createErr);
    m_pboard = nullptr;
    return;
  }

  OSStatus syncErr = PasteboardSynchronize(m_pboard);
  if (syncErr != noErr) {
    LOG_WARN("failed to syncronize clipboard: error %i", syncErr);
  }
}

OSXClipboard::~OSXClipboard()
{
  clearConverters();
}

bool OSXClipboard::empty()
{
  LOG_DEBUG("emptying clipboard");
  if (m_pboard == nullptr)
    return false;

  OSStatus err = PasteboardClear(m_pboard);
  if (err != noErr) {
    LOG_WARN("failed to clear clipboard: error %i", err);
    return false;
  }

  return true;
}

bool OSXClipboard::synchronize()
{
  if (m_pboard == nullptr)
    return false;

  PasteboardSyncFlags flags = PasteboardSynchronize(m_pboard);
  LOG_DEBUG2("flags: %x", flags);

  if (flags & kPasteboardModified) {
    return true;
  }
  return false;
}

void OSXClipboard::add(Format format, const std::string &data)
{
  if (m_pboard == nullptr)
    return;

  LOG_DEBUG("add %d bytes to clipboard format: %d", data.size(), format);
  if (format == IClipboard::Format::Text) {
    LOG_DEBUG("format of data to be added to clipboard was kText");
  } else if (format == IClipboard::Format::Bitmap) {
    LOG_DEBUG("format of data to be added to clipboard was kBitmap");
  } else if (format == IClipboard::Format::HTML) {
    LOG_DEBUG("format of data to be added to clipboard was kHTML");
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
        LOG_DEBUG("added %d bytes to clipboard format: %d", data.size(), format);
      }
    }
  }
}

bool OSXClipboard::open(Time time) const
{
  if (m_pboard == nullptr)
    return false;

  LOG_DEBUG("opening clipboard");
  m_time = time;
  return true;
}

void OSXClipboard::close() const
{
  LOG_DEBUG("closing clipboard");
  /* not needed */
}

IClipboard::Time OSXClipboard::getTime() const
{
  return m_time;
}

bool OSXClipboard::has(Format format) const
{
  if (m_pboard == nullptr)
    return false;

  ItemCount itemCount = 0;
  if (PasteboardGetItemCount(m_pboard, &itemCount) != noErr || itemCount <= 0) {
    return false;
  }

  for (ItemCount i = 1; i <= itemCount; ++i) {
    PasteboardItemID item = 0;
    if (PasteboardGetItemIdentifier(m_pboard, i, &item) != noErr) {
      continue;
    }

    for (ConverterList::const_iterator index = m_converters.begin(); index != m_converters.end(); ++index) {
      IOSXClipboardConverter *converter = *index;
      if (converter->getFormat() != format) {
        continue;
      }

      PasteboardFlavorFlags flags;
      CFStringRef type = converter->getOSXFormat();

      if (PasteboardGetItemFlavorFlags(m_pboard, item, type, &flags) == noErr) {
        return true;
      }
    }
  }

  return false;
}

std::string OSXClipboard::get(Format format) const
{
  if (m_pboard == nullptr)
    return {};

  ItemCount itemCount = 0;
  if (PasteboardGetItemCount(m_pboard, &itemCount) != noErr || itemCount <= 0) {
    return {};
  }

  for (ItemCount i = 1; i <= itemCount; ++i) {
    PasteboardItemID item = 0;
    if (PasteboardGetItemIdentifier(m_pboard, i, &item) != noErr) {
      continue;
    }

    // try converters in order and keep going if a converter cannot decode
    // non-empty data. this allows fallback between TIFF/PNG/BMP flavors.
    for (ConverterList::const_iterator index = m_converters.begin(); index != m_converters.end(); ++index) {
      IOSXClipboardConverter *converter = *index;

      if (converter->getFormat() != format) {
        continue;
      }

      CFStringRef type = converter->getOSXFormat();
      PasteboardFlavorFlags flags;
      if (PasteboardGetItemFlavorFlags(m_pboard, item, type, &flags) != noErr) {
        continue;
      }

      // Some providers advertise image flavors before data is ready.
      // Retry briefly to avoid user-visible copy retries.
      const int maxAttempts = (format == IClipboard::Format::Bitmap) ? 3 : 1;
      for (int attempt = 0; attempt < maxAttempts; ++attempt) {
        std::string rawData;
        if (!readPasteboardFlavorData(m_pboard, item, type, rawData)) {
          break;
        }

        try {
          const auto converted = converter->toIClipboard(rawData);
          if (!converted.empty() || rawData.empty()) {
            return converted;
          }
        } catch (...) {
          LOG_DEBUG("exception while converting pasteboard flavor");
          break;
        }

        if (attempt + 1 < maxAttempts) {
          std::this_thread::sleep_for(std::chrono::milliseconds(35));
        }
      }

      LOG_DEBUG("converter returned no data for available flavor, trying next");
    }
  }

  LOG_DEBUG("unable to find converter for data");
  return {};
}

void OSXClipboard::clearConverters()
{
  if (m_pboard == nullptr)
    return;

  for (ConverterList::iterator index = m_converters.begin(); index != m_converters.end(); ++index) {
    delete *index;
  }
  m_converters.clear();
}

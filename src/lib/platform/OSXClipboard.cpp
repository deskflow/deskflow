/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016, 2026 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/OSXClipboard.h"

#include "arch/ArchException.h"
#include "base/Log.h"
#include "platform/OSXClipboardBMPConverter.h"
#include "platform/OSXClipboardHTMLConverter.h"
#include "platform/OSXClipboardImageConverter.h"
#include "platform/OSXClipboardRawConverter.h"
#include "platform/OSXClipboardTextConverter.h"
#include "platform/OSXClipboardUTF16Converter.h"
#include "platform/OSXClipboardUTF8Converter.h"

#include <algorithm>

//
// OSXClipboard
//

OSXClipboard::OSXClipboard() : m_time(0), m_pboard(nullptr)
{
  m_converters.push_back(new OSXClipboardHTMLConverter);

  // image files go under both the standard type and the one qt apps read
  m_converters.push_back(new OSXClipboardRawConverter("public.png", IClipboard::Format::PNG));
  m_converters.push_back(new OSXClipboardRawConverter(kQtPngType, IClipboard::Format::PNG));
  m_converters.push_back(new OSXClipboardRawConverter("public.jpeg", IClipboard::Format::JPEG));
  m_converters.push_back(new OSXClipboardRawConverter("com.trolltech.anymime.image--jpeg", IClipboard::Format::JPEG));
  m_converters.push_back(new OSXClipboardRawConverter("org.webmproject.webp", IClipboard::Format::WebP));
  m_converters.push_back(new OSXClipboardRawConverter("com.trolltech.anymime.image--webp", IClipboard::Format::WebP));
  m_converters.push_back(new OSXClipboardRawConverter("public.tiff", IClipboard::Format::TIFF));
  m_converters.push_back(new OSXClipboardRawConverter("com.trolltech.anymime.image--tiff", IClipboard::Format::TIFF));
  m_converters.push_back(new OSXClipboardRawConverter("com.compuserve.gif", IClipboard::Format::GIF));
  m_converters.push_back(new OSXClipboardRawConverter("com.trolltech.anymime.image--gif", IClipboard::Format::GIF));
  m_converters.push_back(new OSXClipboardRawConverter("public.svg-image", IClipboard::Format::SVG));
  m_converters.push_back(new OSXClipboardRawConverter("com.trolltech.anymime.image--svg+xml", IClipboard::Format::SVG));

  // raw pixels: older peers' bitmaps are written as png, and the rest are read to send as png
  m_converters.push_back(new OSXClipboardImageConverter("public.png", kQtPngType));
  m_converters.push_back(new OSXClipboardImageConverter("public.heic"));
  m_converters.push_back(new OSXClipboardImageConverter("com.trolltech.anymime.image--bmp"));
  m_converters.push_back(new OSXClipboardBMPConverter);

  m_converters.push_back(new OSXClipboardRawConverter("public.rtf", IClipboard::Format::RTF));
  m_converters.push_back(new OSXClipboardRawConverter("com.trolltech.anymime.text--rtf", IClipboard::Format::RTF));

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

  m_flavors.reset();

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
  LOG_VERBOSE("flags: %x", flags);

  if (flags & kPasteboardModified) {
    return true;
  }
  return false;
}

void OSXClipboard::add(Format format, const std::string &data)
{
  if (m_pboard == nullptr)
    return;

  const auto size = formatSize(data.size());
  LOG_DEBUG("adding to clipboard, format: %d, size: %s", format, size.constData());
  if (format == IClipboard::Format::Text) {
    LOG_DEBUG("format of data to be added to clipboard was kText");
  } else if (format == IClipboard::Format::Bitmap) {
    LOG_DEBUG("format of data to be added to clipboard was kBitmap");
  } else if (format == IClipboard::Format::HTML) {
    LOG_DEBUG("format of data to be added to clipboard was kHTML");
  }

  // macos converts other flavours on demand, but native and qt apps look for unchanged files under different types
  const bool writeEveryFlavour = isFile(format);
  bool added = false;
  for (ConverterList::const_iterator index = m_converters.begin();
       index != m_converters.end() && (!added || writeEveryFlavour); ++index) {
    IOSXClipboardConverter *converter = *index;
    if (converter->getFormat() == format) {
      std::string osXData = converter->fromIClipboard(data);
      if (osXData.empty()) {
        continue;
      }

      CFStringRef flavorType = converter->getOSXFormat();
      CFDataRef dataRef = CFDataCreate(kCFAllocatorDefault, (uint8_t *)osXData.data(), osXData.size());
      PasteboardItemID itemID = 0;

      if (dataRef) {
        PasteboardPutItemFlavor(m_pboard, itemID, flavorType, dataRef, kPasteboardFlavorNoFlags);
        if (CFStringRef alias = converter->getAliasOSXFormat(); alias != nullptr) {
          PasteboardPutItemFlavor(m_pboard, itemID, alias, dataRef, kPasteboardFlavorNoFlags);
        }

        CFRelease(dataRef);
        LOG_DEBUG("added to clipboard, format: %d, size: %s", format, size.constData());
        added = true;
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
  m_flavors.reset();
  return true;
}

void OSXClipboard::close() const
{
  LOG_DEBUG("closing clipboard");
  m_flavors.reset();
}

IClipboard::Time OSXClipboard::getTime() const
{
  return m_time;
}

bool OSXClipboard::has(Format format) const
{
  const auto available = [this](Format candidate) { return findConverter(candidate) != nullptr; };
  return m_pboard != nullptr && sourceToSend(format, available).has_value();
}

std::string OSXClipboard::get(Format format) const
{
  if (m_pboard == nullptr)
    return {};

  const auto available = [this](Format candidate) { return findConverter(candidate) != nullptr; };
  const auto source = sourceToSend(format, available);
  std::string data;
  if (source == Format::Bitmap) {
    data = OSXClipboardImageConverter("public.png").fromIClipboard(read(findConverter(Format::Bitmap)));
  } else if (source) {
    data = read(findConverter(*source));
  } else {
    LOG_DEBUG("unable to find converter for data");
  }
  return data;
}

IOSXClipboardConverter *OSXClipboard::findConverter(Format format) const
{
  PasteboardItemID item;
  PasteboardGetItemIdentifier(m_pboard, (CFIndex)1, &item);

  // each flavour query can make the copying app produce its data again, so list the flavours once per read
  if (!m_flavors) {
    CFArrayRef flavors = nullptr;
    if (PasteboardCopyItemFlavors(m_pboard, item, &flavors) == noErr)
      m_flavors.reset(flavors);
    else
      LOG_DEBUG("failed to list clipboard flavours");
  }

  // macos converts between types on request, which would hide which file was copied
  const bool skipConverted = isFile(format);
  const auto found = std::ranges::find_if(m_converters, [this, item, format, skipConverted](auto *converter) {
    const auto type = converter->getOSXFormat();
    PasteboardFlavorFlags flags = kPasteboardFlavorNoFlags;
    return converter->getFormat() == format && m_flavors &&
           CFArrayContainsValue(m_flavors.get(), CFRangeMake(0, CFArrayGetCount(m_flavors.get())), type) &&
           PasteboardGetItemFlavorFlags(m_pboard, item, type, &flags) == noErr &&
           !(skipConverted && (flags & kPasteboardFlavorSystemTranslated));
  });
  return found != m_converters.end() ? *found : nullptr;
}

std::string OSXClipboard::read(const IOSXClipboardConverter *converter) const
{
  if (converter == nullptr) {
    LOG_DEBUG("clipboard changed while reading it");
    return {};
  }

  PasteboardItemID item;
  PasteboardGetItemIdentifier(m_pboard, (CFIndex)1, &item);

  std::string result;
  CFDataRef buffer = nullptr;
  try {
    OSStatus err = PasteboardCopyItemFlavorData(m_pboard, item, converter->getOSXFormat(), &buffer);

    if (err != noErr) {
      throw err;
    }

    result = std::string((char *)CFDataGetBytePtr(buffer), CFDataGetLength(buffer));
  } catch (OSStatus err) {
    LOG_DEBUG("failed to read clipboard flavour, error: %d", err);
  } catch (...) {
    LOG_DEBUG("unknown exception reading clipboard flavour");
    RETHROW_THREADEXCEPTION
  }

  if (buffer != nullptr)
    CFRelease(buffer);

  return converter->toIClipboard(result);
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

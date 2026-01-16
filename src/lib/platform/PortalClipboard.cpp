/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "PortalClipboard.h"
#include "PortalClipboardProxy.h"
#include "base/Log.h"

namespace deskflow {

PortalClipboard::PortalClipboard(PortalClipboardProxy *proxy, QObject *parent)
  : QObject(parent)
  , m_proxy(proxy)
{
  connect(m_proxy, &PortalClipboardProxy::selectionOwnerChanged, this, &PortalClipboard::onSelectionOwnerChanged);
  connect(m_proxy, &PortalClipboardProxy::selectionTransferRequested, this, &PortalClipboard::onSelectionTransferRequested);
}

PortalClipboard::~PortalClipboard()
{
}

bool PortalClipboard::empty()
{
  if (!m_isOpen) {
    return false;
  }
  
  std::lock_guard<std::mutex> lock(m_cacheMutex);
  m_cache.clear();
  return true;
}

void PortalClipboard::add(Format format, const std::string &data)
{
  if (!m_isOpen) {
    return;
  }
  
  std::lock_guard<std::mutex> lock(m_cacheMutex);
  m_cache[format] = data;
}

bool PortalClipboard::open(Time time) const
{
  if (m_isOpen) {
    return false;
  }
  
  m_isOpen = true;
  m_time = time;
  
  return true;
}

void PortalClipboard::close() const
{
  if (!m_isOpen) {
    return;
  }
  
  {
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    if (!m_cache.isEmpty() && !m_portalIsOwner) {
      QStringList mimeTypes;
      for (auto it = m_cache.begin(); it != m_cache.end(); ++it) {
        mimeTypes << formatToMimeType(it.key());
      }
      m_proxy->setSelection(mimeTypes);
    }
  }
  
  m_isOpen = false;
}

IClipboard::Time PortalClipboard::getTime() const
{
  return m_time;
}

bool PortalClipboard::has(Format format) const
{
  if (!m_isOpen) {
    return false;
  }
  
  std::lock_guard<std::mutex> lock(m_cacheMutex);
  if (m_cache.contains(format)) {
    return true;
  }
  
  QString mimeType = formatToMimeType(format);
  return m_portalMimeTypes.contains(mimeType);
}

std::string PortalClipboard::get(Format format) const
{
  if (!m_isOpen) {
    return "";
  }
  
  std::lock_guard<std::mutex> lock(m_cacheMutex);
  if (m_cache.contains(format)) {
    return m_cache[format];
  }
  
  QString mimeType = formatToMimeType(format);
  if (m_portalMimeTypes.contains(mimeType)) {
    QByteArray data = m_proxy->readSelectionData(mimeType);
    std::string strData(data.constData(), (size_t)data.size());
    m_cache[format] = strData;
    return strData;
  }
  
  return "";
}

void PortalClipboard::onSelectionOwnerChanged(const QStringList &mimeTypes, bool sessionIsOwner)
{
  LOG_DEBUG("Portal clipboard ownership changed: owner=%d, types=%s", sessionIsOwner, mimeTypes.join(", ").toStdString().c_str());
  
  std::lock_guard<std::mutex> lock(m_cacheMutex);
  m_portalMimeTypes = mimeTypes;
  m_portalIsOwner = sessionIsOwner;
  
  if (!sessionIsOwner) {
    m_cache.clear();
  }
}

void PortalClipboard::onSelectionTransferRequested(const QString &mimeType, quint32 serial)
{
  LOG_DEBUG("Portal requested selection transfer for %s (serial %u)", mimeType.toStdString().c_str(), serial);
  
  Format format = mimeTypeToFormat(mimeType);
  std::string data;
  
  {
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    if (m_cache.contains(format)) {
      data = m_cache[format];
    }
  }
  
  if (!data.empty()) {
    QByteArray qData(data.data(), (int)data.size());
    m_proxy->writeSelectionData(serial, qData);
  } else {
    LOG_WARN("Requested MIME type %s not found in cache", mimeType.toStdString().c_str());
    m_proxy->selectionWriteDone(serial, false);
  }
}

QString PortalClipboard::formatToMimeType(Format format) const
{
  switch (format) {
    case Format::Text:   return "text/plain;charset=utf-8";
    case Format::HTML:   return "text/html";
    case Format::Bitmap: return "image/bmp";
    default:             return "";
  }
}

IClipboard::Format PortalClipboard::mimeTypeToFormat(const QString &mimeType) const
{
  if (mimeType.contains("text/plain", Qt::CaseInsensitive)) {
    return Format::Text;
  }
  if (mimeType.contains("text/html", Qt::CaseInsensitive)) {
    return Format::HTML;
  }
  if (mimeType.contains("image/bmp", Qt::CaseInsensitive) || 
      mimeType.contains("image/png", Qt::CaseInsensitive)) {
    return Format::Bitmap;
  }
  return Format::TotalFormats;
}

void PortalClipboard::updateCacheFromPortal() const
{
}

} // namespace deskflow

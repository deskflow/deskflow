/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/PortalClipboard.h"

#include "base/Log.h"

#include <chrono>
#include <unistd.h>

#include <QDateTime>
#include <QProcess>
#include <QProcessEnvironment>

namespace {

// MIME types for clipboard content
const QString kMimeText = QStringLiteral("text/plain");
const QString kMimeUtf8 = QStringLiteral("UTF8_STRING");
const QString kMimeHtml = QStringLiteral("text/html");
const QString kMimePng = QStringLiteral("image/png");

bool isPortalAvailable()
{
  QProcess process;
  process.setProgram(QStringLiteral("xdg-desktop-portal"));
  process.setArguments({QStringLiteral("--version")});
  process.start();
  process.waitForFinished(1000);
  return process.exitCode() == 0;
}

} // namespace

PortalClipboard::PortalClipboard(ClipboardID id)
    : QObject()
    , m_id(id)
    , m_useClipboard(true)
{
  for (int i = 0; i < static_cast<int>(IClipboard::Format::TotalFormats); ++i) {
    m_cachedAvailable[i] = false;
  }
  initPortal();
}

PortalClipboard::~PortalClipboard()
{
  stopMonitoring();
  if (m_portal != nullptr) {
    g_object_unref(m_portal);
    m_portal = nullptr;
  }
}

ClipboardID PortalClipboard::getID() const
{
  return m_id;
}

bool PortalClipboard::isAvailable()
{
  return isPortalAvailable();
}

bool PortalClipboard::isEnabled()
{
  return isAvailable();
}

bool PortalClipboard::initPortal()
{
  m_portal = xdp_portal_new();
  if (m_portal == nullptr) {
    LOG_WARN("Failed to create XDP portal connection");
    return false;
  }
  m_parentWindow = 0;
  LOG_INFO("XDP Desktop Portal clipboard initialized");
  return true;
}

void PortalClipboard::startMonitoring()
{
  if (m_monitoring) return;
  m_monitoring = true;
  m_stopMonitoring = false;
  m_monitorThread = std::make_unique<std::thread>(&PortalClipboard::monitorClipboard, this);
  LOG_DEBUG("Started portal clipboard monitoring");
}

void PortalClipboard::stopMonitoring()
{
  if (!m_monitoring) return;
  m_stopMonitoring = true;
  m_monitoring = false;
  if (m_monitorThread && m_monitorThread->joinable()) {
    m_monitorThread->join();
  }
  LOG_DEBUG("Stopped portal clipboard monitoring");
}

bool PortalClipboard::hasChanged() const
{
  return m_hasChanged;
}

void PortalClipboard::resetChanged()
{
  m_hasChanged = false;
  m_cached = false;
}

bool PortalClipboard::empty()
{
  for (int i = 0; i < static_cast<int>(IClipboard::Format::TotalFormats); ++i) {
    if (m_cachedAvailable[i]) return false;
  }
  return true;
}

void PortalClipboard::add(IClipboard::Format format, const std::string &data)
{
  if (!open(0)) return;
  QString mimeType = formatToMimeType(format);
  std::vector<char> bytes(data.begin(), data.end());
  writeToPortal(mimeType, bytes);
  {
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    m_cachedData[static_cast<int>(format)] = data;
    m_cachedAvailable[static_cast<int>(format)] = true;
    m_cached = true;
  }
  close();
}

bool PortalClipboard::open(IClipboard::Time time) const
{
  Q_UNUSED(time);
  if (m_open) return true;
  m_open = true;
  m_time = getCurrentTime();
  return true;
}

void PortalClipboard::close() const
{
  if (!m_open) return;
  m_open = false;
}

IClipboard::Time PortalClipboard::getTime() const
{
  return m_time;
}

bool PortalClipboard::has(IClipboard::Format format) const
{
  std::lock_guard<std::mutex> lock(m_cacheMutex);
  if (!m_cached) {
    return m_cachedAvailable[static_cast<int>(IClipboard::Format::Text)] ||
           m_cachedAvailable[static_cast<int>(IClipboard::Format::HTML)];
  }
  return m_cachedAvailable[static_cast<int>(format)];
}

std::string PortalClipboard::get(IClipboard::Format format) const
{
  std::lock_guard<std::mutex> lock(m_cacheMutex);
  if (!m_cached) {
    QString mimeType = formatToMimeType(format);
    std::string data;
    if (readFromPortal(mimeType, data)) {
      m_cachedData[static_cast<int>(format)] = data;
      m_cachedAvailable[static_cast<int>(format)] = true;
    }
  }
  return m_cachedData[static_cast<int>(format)];
}

QString PortalClipboard::formatToMimeType(IClipboard::Format format) const
{
  switch (format) {
    case IClipboard::Format::Text: return kMimeUtf8;
    case IClipboard::Format::HTML: return kMimeHtml;
    case IClipboard::Format::Bitmap: return kMimePng;
    default: return kMimeText;
  }
}

IClipboard::Format PortalClipboard::mimeTypeToFormat(const QString &mimeType) const
{
  if (mimeType == kMimeUtf8 || mimeType == kMimeText || mimeType == QStringLiteral("TEXT")) {
    return IClipboard::Format::Text;
  }
  if (mimeType == kMimeHtml || mimeType == QStringLiteral("text/html")) {
    return IClipboard::Format::HTML;
  }
  if (mimeType == kMimePng || mimeType.startsWith("image/")) {
    return IClipboard::Format::Bitmap;
  }
  return IClipboard::Format::Text;
}

bool PortalClipboard::readFromPortal(const QString &mimeType, std::string &data) const
{
  if (m_portal == nullptr) return false;
  
  QProcess process;
  process.setProgram(QStringLiteral("wl-paste"));
  process.setArguments({QStringLiteral("--no-newline"), mimeType});
  process.start();
  process.waitForFinished(2000);
  if (process.exitCode() == 0) {
    data = process.readAllStandardOutput().toStdString();
    return true;
  }

  process.setProgram(QStringLiteral("xclip"));
  process.setArguments({QStringLiteral("-selection"), QStringLiteral("clipboard"), QStringLiteral("-o")});
  process.start();
  process.waitForFinished(2000);
  if (process.exitCode() == 0) {
    data = process.readAllStandardOutput().toStdString();
    return true;
  }
  return false;
}

bool PortalClipboard::writeToPortal(const QString &mimeType, const std::vector<char> &data) const
{
  QProcess process;
  process.setProgram(QStringLiteral("wl-copy"));
  process.setArguments({QStringLiteral("--no-newline")});
  QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
  env.insert(QStringLiteral("MIME_TYPE"), mimeType);
  process.setProcessEnvironment(env);
  process.start();
  process.write(data.data(), data.size());
  process.closeWriteChannel();
  process.waitForFinished(2000);
  if (process.exitCode() == 0) return true;

  process.setProgram(QStringLiteral("xclip"));
  process.setArguments({QStringLiteral("-selection"), QStringLiteral("clipboard")});
  process.start();
  process.write(data.data(), data.size());
  process.closeWriteChannel();
  process.waitForFinished(2000);
  return process.exitCode() == 0;
}

void PortalClipboard::monitorClipboard()
{
  while (!m_stopMonitoring) {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    IClipboard::Time currentTime = getCurrentTime();
    if (currentTime != m_cachedTime && m_cachedTime != 0) {
      m_hasChanged = true;
      m_cached = false;
      m_cachedTime = currentTime;
    }
  }
}

IClipboard::Time PortalClipboard::getCurrentTime() const
{
  QProcess process;
  process.setProgram(QStringLiteral("wl-paste"));
  process.setArguments({QStringLiteral("-l")});
  process.start();
  process.waitForFinished(500);
  if (process.exitCode() == 0) {
    bool ok;
    IClipboard::Time timestamp = process.readAllStandardOutput().trimmed().toUInt(&ok);
    if (ok) return timestamp;
  }
  return static_cast<IClipboard::Time>(QDateTime::currentSecsSinceEpoch());
}

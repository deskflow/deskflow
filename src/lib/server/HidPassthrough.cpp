/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "server/HidPassthrough.h"

#include "base/IEventQueue.h"
#include "base/Log.h"
#include "common/Settings.h"

#include <QJsonDocument>
#include <QJsonObject>

#include <cctype>

namespace deskflow::server {

namespace {

//! Parse one hex field ("046D"); returns -1 when malformed.
int parseHexField(const std::string &field)
{
  if (field.empty() || field.size() > 4) {
    return -1;
  }
  int value = 0;
  for (const char raw : field) {
    const char c = static_cast<char>(std::tolower(static_cast<unsigned char>(raw)));
    int digit = -1;
    if (c >= '0' && c <= '9') {
      digit = c - '0';
    } else if (c >= 'a' && c <= 'f') {
      digit = c - 'a' + 10;
    } else {
      return -1;
    }
    value = (value << 4) | digit;
  }
  return value;
}

std::string trimmed(const std::string &value)
{
  const auto first = value.find_first_not_of(" \t\r\n");
  if (first == std::string::npos) {
    return {};
  }
  const auto last = value.find_last_not_of(" \t\r\n");
  return value.substr(first, last - first + 1);
}

std::string toHex4(uint16_t value)
{
  static const char *digits = "0123456789ABCDEF";
  std::string out = "0x";
  for (int shift = 12; shift >= 0; shift -= 4) {
    out.push_back(digits[(value >> shift) & 0xF]);
  }
  return out;
}

} // namespace

std::vector<HidDeviceSelector> parseHidDeviceSelectors(const std::string &setting)
{
  std::vector<HidDeviceSelector> selectors;
  std::string::size_type start = 0;
  while (start <= setting.size()) {
    auto end = setting.find(',', start);
    if (end == std::string::npos) {
      end = setting.size();
    }
    const std::string entry = trimmed(setting.substr(start, end - start));
    start = end + 1;
    if (entry.empty()) {
      continue;
    }

    const auto colon = entry.find(':');
    if (colon == std::string::npos || colon == 0 || colon == entry.size() - 1) {
      continue; // malformed: need VID:PID
    }
    const int vid = parseHexField(trimmed(entry.substr(0, colon)));
    const std::string pidField = trimmed(entry.substr(colon + 1));
    const int pid = pidField == "*" ? 0 : parseHexField(pidField);
    if (vid <= 0 || pid < 0) {
      continue;
    }
    selectors.push_back({static_cast<uint16_t>(vid), static_cast<uint16_t>(pid)});
  }
  return selectors;
}

std::string hidBytesToHex(const uint8_t *bytes, size_t len)
{
  static const char *digits = "0123456789abcdef";
  std::string out;
  out.reserve(len * 2);
  for (size_t i = 0; i < len; ++i) {
    out.push_back(digits[bytes[i] >> 4]);
    out.push_back(digits[bytes[i] & 0xF]);
  }
  return out;
}

HidPassthrough::HidPassthrough(IEventQueue *events, void *eventTarget) : m_events(events), m_eventTarget(eventTarget)
{
  // do nothing
}

HidPassthrough::~HidPassthrough()
{
  stop();
}

bool HidPassthrough::start()
{
  const auto setting = Settings::value(Settings::Server::HidPassthroughDevices).toString().toStdString();
  const auto selectors = parseHidDeviceSelectors(setting);
  if (selectors.empty()) {
    LOG_WARN("hid passthrough enabled but no valid devices configured (\"%s\")", setting.c_str());
    return false;
  }

  m_grabber = createHidGrabber();
  const bool ok = m_grabber->start(
      selectors,
      [this](const HidDeviceDescriptor &descriptor) {
        m_events->addEvent(Event(
            EventTypes::ServerHidPassthroughEvent, m_eventTarget,
            new HidPassthroughEventData(
                HidPassthroughEventData::Kind::Attach, connectLineFor(descriptor), descriptor.deviceId
            )
        ));
      },
      [this](uint16_t deviceId, std::string bytes) {
        m_events->addEvent(Event(
            EventTypes::ServerHidPassthroughEvent, m_eventTarget,
            new HidPassthroughEventData(HidPassthroughEventData::Kind::Frame, std::move(bytes), deviceId)
        ));
      },
      [this](uint16_t deviceId) {
        m_events->addEvent(Event(
            EventTypes::ServerHidPassthroughEvent, m_eventTarget,
            new HidPassthroughEventData(HidPassthroughEventData::Kind::Detach, R"({"type": "disconnect"})", deviceId)
        ));
      }
  );
  if (!ok) {
    m_grabber.reset();
    return false;
  }
  m_started = true;
  LOG_INFO("hid passthrough started (%d device selector(s))", static_cast<int>(selectors.size()));
  return true;
}

void HidPassthrough::stop()
{
  if (m_grabber) {
    m_grabber->stop();
    m_grabber.reset();
  }
  m_started = false;
}

void HidPassthrough::setFocusRemote(bool remote)
{
  if (m_started && m_grabber) {
    m_grabber->setSeized(remote);
  }
}

std::string HidPassthrough::connectLineFor(const HidDeviceDescriptor &descriptor) const
{
  // Consumer-protocol "connect" line (same vocabulary the Mouser bridge
  // relays), so the focused client's existing loopback consumer accepts
  // the virtual device without new plumbing.
  QJsonObject device;
  device[QStringLiteral("product_id")] = QString::fromStdString(toHex4(descriptor.pid));
  device[QStringLiteral("vendor_id")] = QString::fromStdString(toHex4(descriptor.vid));
  if (!descriptor.name.empty()) {
    device[QStringLiteral("product_name")] = QString::fromStdString(descriptor.name);
  }
  device[QStringLiteral("usage_page")] = static_cast<int>(descriptor.usagePage);
  device[QStringLiteral("usage")] = static_cast<int>(descriptor.usage);
  device[QStringLiteral("source")] = QStringLiteral("deskflow-passthrough");

  QJsonObject line;
  line[QStringLiteral("type")] = QStringLiteral("connect");
  line[QStringLiteral("device")] = device;
  return QJsonDocument(line).toJson(QJsonDocument::Compact).toStdString();
}

} // namespace deskflow::server

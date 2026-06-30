/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "server/HidppProbe.h"

#include "base/Log.h"

#include <QString>

namespace deskflow::server {

namespace {

struct CatalogEntry
{
  uint16_t pid;
  uint16_t gestureCid;
  std::map<uint16_t, std::string> extraDiverts;
};

const CatalogEntry kCatalog[] = {
    // MX Master 4
    {0xB042, 0x01A0, {{0x00C4, "thumb_button"}}},
    // MX Master 3S
    {0xB034, 0x00C3, {{0x00C4, "thumb_button"}}},
    // MX Master 3
    {0xB023, 0x00C3, {{0x00C4, "thumb_button"}}},
};

const CatalogEntry *catalogForPid(uint16_t productId)
{
  for (const auto &entry : kCatalog) {
    if (entry.pid == productId) {
      return &entry;
    }
  }
  return nullptr;
}

void applyCatalogDefaults(HidppDecodeContext &context, uint16_t productId)
{
  const CatalogEntry *entry = catalogForPid(productId);
  if (entry == nullptr) {
    if (context.gestureCid == 0) {
      context.gestureCid = 0x00C3;
    }
    return;
  }
  if (context.gestureCid == 0) {
    context.gestureCid = entry->gestureCid;
  }
  if (context.extraDiverts.empty()) {
    context.extraDiverts = entry->extraDiverts;
  }
}

} // namespace

#if defined(__APPLE__) || defined(_WIN32)
HidppDecodeContext probeHidppDecodePlatform(uint16_t vendorId, uint16_t productId);
#else
namespace {
HidppDecodeContext probeHidppDecodePlatform(uint16_t, uint16_t)
{
  return {};
}
} // namespace
#endif

bool hidppProbeCapable()
{
#if defined(__APPLE__) || defined(_WIN32)
  return true;
#else
  return false;
#endif
}

HidppDecodeContext probeHidppDecode(uint16_t vendorId, uint16_t productId)
{
  if (vendorId == 0) {
    vendorId = 0x046D;
  }
  auto context = probeHidppDecodePlatform(vendorId, productId);
  if (!context.valid()) {
    LOG_DEBUG("hidpp probe: no feat_idx for VID=0x%04X PID=0x%04X", vendorId, productId);
    return {};
  }
  applyCatalogDefaults(context, productId);
  LOG_INFO(
      "hidpp probe: feat_idx=0x%02X gesture_cid=0x%04X for PID=0x%04X", context.featIdx, context.gestureCid, productId
  );
  return context;
}

QJsonObject hidppDecodeToJson(const HidppDecodeContext &context)
{
  QJsonObject decode;
  if (!context.valid()) {
    return decode;
  }
  decode[QStringLiteral("feat_idx")] = context.featIdx;
  decode[QStringLiteral("gesture_cid")] =
      QStringLiteral("0x%1").arg(context.gestureCid, 4, 16, QLatin1Char('0'));
  decode[QStringLiteral("rawxy")] = context.rawxy;
  if (!context.extraDiverts.empty()) {
    QJsonObject extra;
    for (const auto &[cid, role] : context.extraDiverts) {
      extra[QStringLiteral("0x%1").arg(cid, 4, 16, QLatin1Char('0'))] = QString::fromStdString(role);
    }
    decode[QStringLiteral("extra_diverts")] = extra;
  }
  return decode;
}

} // namespace deskflow::server

// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "DisplayTopology.h"
#include <QCryptographicHash>
#include <QDebug>
#include <QGuiApplication>
#include <QMetaObject>
#include <QScreen>
#include <QThread>
#ifdef Q_OS_MACOS
#include <ColorSync/ColorSync.h>
#include <CoreGraphics/CoreGraphics.h>
#endif
#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <QtGui/qscreen_platform.h>
#include <windows.h>
#endif

namespace deskflow::display {
namespace {
QString opaqueId(const QByteArray &osId)
{
  return QString::fromLatin1(QCryptographicHash::hash(osId, QCryptographicHash::Sha256).toHex());
}
Topology capture()
{
  Topology result;
#ifdef Q_OS_MACOS
  CGDirectDisplayID displays[16];
  uint32_t count = 0;
  const auto status = CGGetActiveDisplayList(16, displays, &count);
  if (status != kCGErrorSuccess || !count) {
    qWarning() << "display capture unavailable: active list" << status << count;
    return {};
  }
  for (uint32_t i = 0; i < count; ++i) {
    const auto id = displays[i];
    if (CGDisplayMirrorsDisplay(id) != kCGNullDirectDisplay)
      continue;
    const auto uuid = CGDisplayCreateUUIDFromDisplayID(id);
    if (!uuid) {
      qWarning("display capture unavailable: UUID");
      return {};
    }
    const auto uuidString = CFUUIDCreateString(kCFAllocatorDefault, uuid);
    CFRelease(uuid);
    if (!uuidString)
      return {};
    char text[128];
    const bool ok = CFStringGetCString(uuidString, text, sizeof(text), kCFStringEncodingUTF8);
    CFRelease(uuidString);
    if (!ok)
      return {};
    const auto bounds = CGDisplayBounds(id);
    const auto mode = CGDisplayCopyDisplayMode(id);
    if (!mode) {
      qWarning("display capture unavailable: mode");
      return {};
    }
    const QSize pixels(int(CGDisplayModeGetPixelWidth(mode)), int(CGDisplayModeGetPixelHeight(mode)));
    CGDisplayModeRelease(mode);
    const QRectF logical(bounds.origin.x, bounds.origin.y, bounds.size.width, bounds.size.height);
    result.monitors.append({opaqueId(QByteArray("mac:") + text), logical, logical.toRect(), pixels, {}});
  }
#elif defined(Q_OS_WIN)
  // Persist the OS target path as a hash only. DISPLAY1/index/geometry are not
  // stable identifiers. No EDID or serial-number field is read or transmitted.
  UINT32 pathCount = 0, modeCount = 0;
  if (GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &pathCount, &modeCount) != ERROR_SUCCESS || pathCount > 32 ||
      modeCount > 96)
    return {};
  QVector<DISPLAYCONFIG_PATH_INFO> paths(pathCount);
  QVector<DISPLAYCONFIG_MODE_INFO> modes(modeCount);
  if (QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS, &pathCount, paths.data(), &modeCount, modes.data(), nullptr) !=
      ERROR_SUCCESS)
    return {};
  QMap<QString, QString> stable;
  for (UINT32 i = 0; i < pathCount; ++i) {
    const auto &path = paths[i];
    DISPLAYCONFIG_SOURCE_DEVICE_NAME source{};
    source.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
    source.header.size = sizeof(source);
    source.header.adapterId = path.sourceInfo.adapterId;
    source.header.id = path.sourceInfo.id;
    DISPLAYCONFIG_TARGET_DEVICE_NAME target{};
    target.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME;
    target.header.size = sizeof(target);
    target.header.adapterId = path.targetInfo.adapterId;
    target.header.id = path.targetInfo.id;
    if (DisplayConfigGetDeviceInfo(&source.header) != ERROR_SUCCESS ||
        DisplayConfigGetDeviceInfo(&target.header) != ERROR_SUCCESS)
      return {};
    const auto name = QString::fromWCharArray(source.viewGdiDeviceName),
               pathName = QString::fromWCharArray(target.monitorDevicePath);
    if (pathName.isEmpty() || stable.contains(name))
      return {}; // Ambiguous clone group: retain legacy mode.
    stable.insert(name, opaqueId(QByteArray("win:") + pathName.toUtf8()));
  }
  struct Context
  {
    Topology *topology;
    const QMap<QString, QString> *stable;
    bool ok = true;
  } context{&result, &stable};
  const auto enumerate = [](HMONITOR handle, HDC, LPRECT, LPARAM data) -> BOOL {
    auto &ctx = *reinterpret_cast<Context *>(data);
    MONITORINFOEXW info{};
    info.cbSize = sizeof(info);
    if (!GetMonitorInfoW(handle, reinterpret_cast<MONITORINFO *>(&info))) {
      ctx.ok = false;
      return FALSE;
    }
    const auto name = QString::fromWCharArray(info.szDevice);
    const auto id = ctx.stable->value(name);
    // QScreen::name() may be a friendly model name (Qt 6.11), not a GDI
    // device name. Match the actual native monitor, preserving the stable
    // target-path identity and each screen's own logical/DPI geometry.
    QScreen *qtScreen = nullptr;
    for (auto *screen : QGuiApplication::screens()) {
      const auto *native = screen->nativeInterface<QNativeInterface::QWindowsScreen>();
      if (native && native->handle() == handle) {
        if (qtScreen) {
          ctx.ok = false;
          return FALSE;
        }
        qtScreen = screen;
      }
    }
    DEVMODEW mode{};
    mode.dmSize = sizeof(mode);
    if (id.isEmpty() || !qtScreen || !EnumDisplaySettingsExW(info.szDevice, ENUM_CURRENT_SETTINGS, &mode, 0)) {
      ctx.ok = false;
      return FALSE;
    }
    const auto r = info.rcMonitor;
    ctx.topology->monitors.append(
        {id,
         qtScreen->geometry(),
         QRect(r.left, r.top, r.right - r.left, r.bottom - r.top),
         QSize(mode.dmPelsWidth, mode.dmPelsHeight),
         {}}
    );
    return TRUE;
  };
  if (!EnumDisplayMonitors(nullptr, nullptr, enumerate, reinterpret_cast<LPARAM>(&context)) || !context.ok)
    return {};
#endif
  // Neither API establishes that physical millimetres are measured rather than
  // estimated. Leave that optional field unknown; connection never depends on it.
  if (!result.error().isEmpty()) {
    qWarning() << "display capture unavailable:" << result.error();
    return {};
  }
  return result;
}
} // namespace
Topology localTopology()
{
  auto *app = qobject_cast<QGuiApplication *>(QCoreApplication::instance());
  if (!app)
    return {};
  if (QThread::currentThread() == app->thread())
    return capture();
  Topology result;
  QMetaObject::invokeMethod(app, [&result] { result = capture(); }, Qt::BlockingQueuedConnection);
  return result;
}
} // namespace deskflow::display

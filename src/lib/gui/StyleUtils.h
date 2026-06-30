/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QDir>
#include <QFileInfoList>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QIcon>
#include <QImage>
#include <QPalette>
#include <QScreen>
#include <QStyleHints>

#include "common/Constants.h"

namespace deskflow::gui {

/**
 * @brief Detects dark mode in a universal manner (all Qt versions).
 * Until better platform support is added, it's more reliable to use the old way (compare text and window lightness),
 * because the newer versions in Qt 6.5+ are not always correct and some return `UnknownScheme`.
 * https://www.qt.io/blog/dark-mode-on-windows-11-with-qt-6.5
 */
inline bool isDarkMode()
{
  const QPalette defaultPalette;
  const auto text = defaultPalette.color(QPalette::WindowText);
  const auto window = defaultPalette.color(QPalette::Window);
  return text.lightness() > window.lightness();
}
/**
 * @brief get a string for the iconMode
 * @returns "dark" or "light"
 */
inline QString iconMode()
{
  return isDarkMode() ? QStringLiteral("dark") : QStringLiteral("light");
}

inline void updateIconTheme()
{
  // Bundled icons live at :/icons/<themeName>/ (see deskflow.qrc). Qt's icon theme
  // engine expects search paths to be the parent of those theme directories.
  const auto themeName = QStringLiteral("%1-%2").arg(kAppId, iconMode());
  const auto iconRoot = QStringLiteral(":/icons");
  if (QIcon::themeName().isEmpty() || QIcon::themeName().startsWith(kAppId))
    QIcon::setThemeName(themeName);
  else
    QIcon::setFallbackThemeName(themeName);
  QIcon::setThemeSearchPaths({iconRoot});
  QIcon::setFallbackSearchPaths({iconRoot});
}

#if defined(Q_OS_MACOS)
/**
 * @brief Build a menu-bar tray icon from bundled SVG resources.
 *
 * Qt's QIcon::setIsMask on complex SVGs produces a solid colored blob in the
 * macOS status item; rasterize first and, for symbolic icons, reduce to a
 * black+alpha template image before marking as mask.
 */
inline QIcon macMenuBarTrayIcon(const QString &resourcePath, bool asTemplate)
{
  const qreal dpr = QGuiApplication::primaryScreen() ? QGuiApplication::primaryScreen()->devicePixelRatio() : 2.0;
  constexpr int logicalSize = 18;
  const int px = qRound(logicalSize * dpr);
  QPixmap pm = QIcon(resourcePath).pixmap(px, px);
  pm.setDevicePixelRatio(dpr);

  if (asTemplate) {
    QImage img = pm.toImage().convertToFormat(QImage::Format_ARGB32);
    for (int y = 0; y < img.height(); ++y) {
      auto *line = reinterpret_cast<QRgb *>(img.scanLine(y));
      for (int x = 0; x < img.width(); ++x) {
        const int alpha = qAlpha(line[x]);
        line[x] = alpha > 16 ? qRgba(0, 0, 0, alpha) : qRgba(0, 0, 0, 0);
      }
    }
    pm = QPixmap::fromImage(img);
    pm.setDevicePixelRatio(dpr);
  }

  QIcon icon;
  icon.addPixmap(pm);
  if (asTemplate) {
    icon.setIsMask(true);
  }
  return icon;
}
#endif
} // namespace deskflow::gui

inline QFont fixedFont()
{
#if defined(Q_OS_WIN)
  QFont f({"Hack", "Liberation Mono", "Monospace", "Andale Mono"});
  f.setStyleHint(QFont::Monospace);
#else
  QFont f = QFontDatabase::systemFont(QFontDatabase::FixedFont);
#endif

#if defined(Q_OS_MAC)
  f.setPointSize(12);
#endif
  return f;
}

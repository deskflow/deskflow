/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QDir>
#include <QFileInfoList>
#include <QGuiApplication>
#include <QPalette>
#include <QStyleHints>

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

/**
 * @brief checkSubDir checks for subdirs in a dir
 * @param path The path to check for subdirs
 * @return list of subdirs
 */
inline QStringList checkSubDir(const QString &path)
{
  QStringList paths;
  auto dir = QDir(path);
  const QFileInfoList items = dir.entryInfoList({"*"}, QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
  for (const QFileInfo &item : items) {
    if (item.isDir()) {
      paths.append(item.absoluteFilePath());
      paths.append(checkSubDir(item.absoluteFilePath()));
    }
  }
  return paths;
}

/**
 * @brief setIconFallbackPaths Set the icon fallback path to our light or dark theme
 */
inline void setIconFallbackPaths()
{
  QStringList paths = checkSubDir(QStringLiteral(":/icons/deskflow-%1").arg(iconMode()));
  QIcon::setFallbackSearchPaths(paths);
}
} // namespace deskflow::gui

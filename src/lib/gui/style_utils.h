/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2024 Symless
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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

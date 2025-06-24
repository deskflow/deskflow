/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QDir>
#include <QFileInfoList>
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

} // namespace deskflow::gui

/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2024 Chris Rizzitello <sithlord48@gmail.com>
 * Copyright (C) 2012 Symless Ltd.
 * Copyright (C) 2008 Volker Lanz (vl@fidra.de)
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

#include <QDialog>

class AboutDialog : public QDialog
{
  Q_OBJECT
public:
  explicit AboutDialog(QWidget *parent = nullptr);
  ~AboutDialog() = default;

private:
  inline static const auto s_lightCopy = QStringLiteral(":/icons/64x64/copy-light.png");
  inline static const auto s_darkCopy = QStringLiteral(":/icons/64x64/copy-dark.png");
  inline static const auto s_lightLogo = QStringLiteral(":/image/logo-light.png");
  inline static const auto s_darkLogo = QStringLiteral(":/image/logo-dark.png");
  inline static const auto s_awesomeDevs = QStringList{
      // Chris is the ultimate creator, and the one who started it all in 2001.
      QStringLiteral("Chris Schoeneman"),

      // Richard and Adam developed CosmoSynergy, the 90's predecessor project.
      QStringLiteral("Richard Lee"),
      QStringLiteral("Adam Feder"),

      // Nick continued the legacy in 2009 started by Chris.
      QStringLiteral("Nick Bolton"),

      // Volker wrote the first version of the GUI (QSynergy) in 2008.
      QStringLiteral("Volker Lanz"),

      // Re-ignited the project in 2008 and rebuilt the community.
      QStringLiteral("Sorin Sbârnea"),

      // Contributors of bug fixes in the early days.
      QStringLiteral("Ryan Breen"),
      QStringLiteral("Guido Poschta"),
      QStringLiteral("Bertrand Landry Hetu"),
      QStringLiteral("Tom Chadwick"),
      QStringLiteral("Brent Priddy"),
      QStringLiteral("Jason Axelson"),
      QStringLiteral("Jake Petroules"),

      // Implemented Wayland support (libei and libportal).
      QStringLiteral("Peter Hutterer"),
      QStringLiteral("Olivier Fourdan"),

      // Symless employees (in order of joining).
      QStringLiteral("Kyle Bloom"),
      QStringLiteral("Daun Chung"),
      QStringLiteral("Serhii Hadzhylov"),
      QStringLiteral("Oleksandr Lysytsia"),
      QStringLiteral("Olena Kutytska"),
      QStringLiteral("Owen Phillips"),
      QStringLiteral("Daniel Evenson"),

      // Prominent Deskflow development community members
      QStringLiteral("Chris Rizzitello"),
  };
};

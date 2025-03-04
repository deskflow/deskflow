/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QDialog>

namespace Ui {
class AboutDialog;
}

class AboutDialog : public QDialog
{
  Q_OBJECT
public:
  explicit AboutDialog(QWidget *parent = nullptr);
  ~AboutDialog() override;

private:
  std::unique_ptr<Ui::AboutDialog> ui;
  void copyVersionText();

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

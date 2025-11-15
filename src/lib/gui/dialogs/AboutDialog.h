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

using namespace Qt::StringLiterals;

class AboutDialog : public QDialog
{
  Q_OBJECT
public:
  explicit AboutDialog(QWidget *parent = nullptr);
  ~AboutDialog() override;

private:
  std::unique_ptr<Ui::AboutDialog> ui;
  void copyVersionText() const;

  inline static const auto s_awesomeDevs = QStringList{
      // Chris is the ultimate creator, and the one who started it all in 2001.
      u"Chris Schoeneman"_s,

      // Richard and Adam developed CosmoSynergy, the 90's predecessor project.
      u"Richard Lee"_s,
      u"Adam Feder"_s,

      // Nick continued the legacy in 2009 started by Chris.
      u"Nick Bolton"_s,

      // Volker wrote the first version of the GUI (QSynergy) in 2008.
      u"Volker Lanz"_s,

      // Re-ignited the project in 2008 and rebuilt the community.
      u"Sorin Sbârnea"_s,

      // Contributors of bug fixes in the early days.
      u"Ryan Breen"_s,
      u"Guido Poschta"_s,
      u"Bertrand Landry Hetu"_s,
      u"Tom Chadwick"_s,
      u"Brent Priddy"_s,
      u"Jason Axelson"_s,
      u"Jake Petroules"_s,

      // Implemented Wayland support (libei and libportal).
      u"Peter Hutterer"_s,
      u"Olivier Fourdan"_s,

      // Symless employees (in order of joining).
      u"Kyle Bloom"_s,
      u"Daun Chung"_s,
      u"Serhii Hadzhylov"_s,
      u"Oleksandr Lysytsia"_s,
      u"Olena Kutytska"_s,
      u"Owen Phillips"_s,
      u"Daniel Evenson"_s,

      // Barrier & Input Leap maintainers
      u"Povilas Kanapickas"_s,
      u"Dom Rodriguez"_s,

      // Deskflow maintainers
      u"Chris Rizzitello"_s,
  };
};

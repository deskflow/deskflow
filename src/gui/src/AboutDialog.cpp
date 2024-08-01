/*
 * synergy -- mouse and keyboard sharing utility
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

#include "AboutDialog.h"

#include <QDateTime>

#if defined(Q_OS_MAC)
#include "OSXHelpers.h"
#endif

#ifdef GIT_SHA_SHORT
const QString kVersionAppend = GIT_SHA_SHORT;
#else
const QString kVersionAppend;
#endif

AboutDialog::AboutDialog(MainWindow *parent, const AppConfig &config)
    : QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
      Ui::AboutDialogBase() {
  setupUi(this);

  QString version(SYNERGY_VERSION);
  if (!kVersionAppend.isEmpty()) {
    version.append(QString(" (%1)").arg(kVersionAppend));
  }

  m_pLabelSynergyVersion->setText(version);

  QString buildDateString = QString::fromLocal8Bit(__DATE__).simplified();
  QDate buildDate = QLocale("en_US").toDate(buildDateString, "MMM d yyyy");
  m_pLabelBuildDate->setText(
      buildDate.toString(QLocale::system().dateFormat(QLocale::LongFormat)));
}

int AboutDialog::exec() {
  m_pDevelopersLabel->setText(getImportantDevelopers());
  m_pCopyrightLabel->setText(getCopyright());
  updateLogo();

  return QDialog::exec();
}

void AboutDialog::updateLogo() const {
#if defined(Q_OS_MAC)
  if (isOSXInterfaceStyleDark()) {
    QPixmap logo(":/res/image/about-dark.png");
    if (!logo.isNull()) {
      m_pLabel_Logo->setPixmap(logo);
    }
  }
#endif
}

QString AboutDialog::getImportantDevelopers() const {
  return QString(
      // The ultimate creator
      "Chris Schoeneman, "

      // Precursor developers
      "Richard Lee, Adam Feder, "

      // Contributors
      "Nick Bolton, Volker Lanz, Ryan Breen, Guido Poschta, "
      "Bertrand Landry Hetu, Tom Chadwick, Brent Priddy, Jason Axelson, "
      "Jake Petroules, Sorin Sbârnea, "

      // Symless employees
      "Kyle Bloom, Daun Chung, Serhii Hadzhylov, "
      "Oleksandr Lysytsia, Olena Kutytska.");
}

QString AboutDialog::getCopyright() const {
  QString buildDateString = QString::fromLocal8Bit(__DATE__).simplified();
  QDate buildDate = QLocale("en_US").toDate(buildDateString, "MMM d yyyy");

  QString copyright("Copyright © 2012-%%YEAR%% Symless Ltd.\n"
                    "Copyright © 2009-2012 Nick Bolton\n"
                    "Copyright © 2002-2009 Chris Schoeneman");
  return copyright.replace(
      QString("%%YEAR%%"), QString::number(buildDate.year()));
}

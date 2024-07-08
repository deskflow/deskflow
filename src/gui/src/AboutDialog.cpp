/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
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
#include "OSXHelpers.h"

AboutDialog::AboutDialog(MainWindow *parent, const AppConfig &config)
    : QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
      Ui::AboutDialogBase() {
  setupUi(this);

  m_versionChecker.setApp(parent->appPath(config.synergycName()));

  QString version = SYNERGY_VERSION;
#ifdef GIT_SHA_SHORT
  version += " (" GIT_SHA_SHORT ")";
#endif

  m_pLabelSynergyVersion->setText(version);

  QString buildDateString = QString::fromLocal8Bit(__DATE__).simplified();
  QDate buildDate = QLocale("en_US").toDate(buildDateString, "MMM d yyyy");
  m_pLabelBuildDate->setText(
      buildDate.toString(QLocale::system().dateFormat(QLocale::LongFormat)));
}

int AboutDialog::exec() {
  m_pDevelopersLabel->setText(getImportantDevelopers());
  m_pCopyrightLabel->setText(getCopyright());
  resizeWindow();
  updateLogo();

  return QDialog::exec();
}

void AboutDialog::resizeWindow() {
  QSize size(600, 310);
  setMaximumSize(size);
  setMinimumSize(size);
  resize(size);
}

void AboutDialog::updateLogo() const {
#if defined(Q_OS_MAC)
  if (isOSXInterfaceStyleDark()) {
    QPixmap logo(":/res/image/about-dark.png");
    if (!logo.isNull()) {
      label_Logo->setPixmap(logo);
    }
  }
#endif
}

QString AboutDialog::getImportantDevelopers() const {
  return QString(
      "Chris Schoeneman, Nick Bolton, Richard Lee, Adam Feder, Volker Lanz, "
      "Ryan Breen, Guido Poschta, Bertrand Landry Hetu, Tom Chadwick, "
      "Brent Priddy, Kyle Bloom, Daun Chung, Serhii Hadzhylov, "
      "Oleksandr Lysytsia, Olena Kutytska, Francisco Magalhães.");
}

QString AboutDialog::getCopyright() const {
  QString buildDateString = QString::fromLocal8Bit(__DATE__).simplified();
  QDate buildDate = QLocale("en_US").toDate(buildDateString, "MMM d yyyy");

  QString copyright("Copyright © 2012-%%YEAR%% Symless Ltd.\n"
                    "Copyright © 2009-2012 Nick Bolton\n"
                    "Copyright © 2002-2009 Chris Schoeneman");
  return copyright.replace(QString("%%YEAR%%"),
                           QString::number(buildDate.year()));
}

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
#include "common/copyright.h"

#include <QDateTime>

#if defined(Q_OS_MAC)
#include "gui/OSXHelpers.h"
#endif

#include "common/version.h"

AboutDialog::AboutDialog(MainWindow *parent)
    : QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
      Ui::AboutDialogBase() {

  setupUi(this);

  this->setFixedSize(this->size());

  QString version = QString::fromStdString(synergy::version());
  m_pLabelSynergyVersion->setText(version);

  QString buildDateString = QString::fromLocal8Bit(__DATE__).simplified();
  QDate buildDate = QLocale("en_US").toDate(buildDateString, "MMM d yyyy");
  m_pLabelBuildDate->setText(
      buildDate.toString(QLocale::system().dateFormat(QLocale::LongFormat)));
}

int AboutDialog::exec() {
  m_pDevelopersLabel->setText(importantDevelopers());
  m_pCopyrightLabel->setText(QString::fromStdString(synergy::copyright()));
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

QString AboutDialog::importantDevelopers() const {
  QStringList awesomePeople;

  // Chris is the ultimate creator, and the one who started it all in 2001.
  awesomePeople << "Chris Schoeneman";

  // Richard and Adam developed CosmoSynergy, the 90's predecessor project.
  awesomePeople << "Richard Lee" << "Adam Feder";

  // Nick continued the legacy in 2009 started by Chris.
  awesomePeople << "Nick Bolton";

  // Volker wrote the first version of the GUI (QSynergy) in 2008.
  awesomePeople << "Volker Lanz";

  // Re-ignited the project in 2008 and rebuilt the community.
  awesomePeople << "Sorin Sbârnea";

  // Contributors of bug fixes in the early days.
  awesomePeople << "Ryan Breen"
                << "Guido Poschta"
                << "Bertrand Landry Hetu"
                << "Tom Chadwick"
                << "Brent Priddy"
                << "Jason Axelson"
                << "Jake Petroules";

  // Symless employees (in order of joining).
  awesomePeople << "Kyle Bloom"
                << "Daun Chung"
                << "Serhii Hadzhylov"
                << "Oleksandr Lysytsia"
                << "Olena Kutytska"
                << "Owen Phillips"
                << "Daniel Evenson";

  for (auto &person : awesomePeople) {
    // prevent names from breaking on the space when wrapped
    person = person.replace(" ", QString::fromUtf8("&nbsp;"));
  }

  return awesomePeople.join(", ") + ".";
}

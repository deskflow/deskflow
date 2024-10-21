/*
 * Deskflow -- mouse and keyboard sharing utility
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
#include "common/version.h"
#include "gui/style_utils.h"

#include <QDateTime>
#include <QGuiApplication>

using namespace deskflow::gui;

//
// AboutDialog::Deps
//

bool AboutDialog::Deps::isDarkMode() const
{
  return ::isDarkMode();
}

//
// AboutDialog
//

AboutDialog::AboutDialog(QMainWindow *parent, std::shared_ptr<Deps> deps)
    : QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
      Ui::AboutDialogBase(),
      m_pDeps(deps)
{

  setupUi(this);

  this->setFixedSize(this->size());

  QString version = QString::fromStdString(deskflow::version());
  m_pLabelDeskflowVersion->setTextInteractionFlags(Qt::TextSelectableByMouse);
  m_pLabelDeskflowVersion->setText(version);

  QString buildDateString = QString::fromLocal8Bit(BUILD_DATE).simplified();
  QDate buildDate = QLocale("en_US").toDate(buildDateString, "yyyy-MM-dd");
  m_pLabelBuildDate->setTextInteractionFlags(Qt::TextSelectableByMouse);
  m_pLabelBuildDate->setText(buildDate.toString(QLocale::system().dateFormat(QLocale::LongFormat)));

  this->setWindowTitle(QString("About %1").arg(DESKFLOW_APP_NAME));
}

int AboutDialog::exec()
{
  m_pDevelopersLabel->setText(importantDevelopers());
  m_pCopyrightLabel->setText(QString::fromStdString(deskflow::copyright()));
  updateLogo();

  return QDialog::exec();
}

void AboutDialog::setLogo(const char *const &filename) const
{
  QPixmap logo(filename);
  if (!logo.isNull()) {
    m_pLabel_Logo->setPixmap(logo);
  } else {
    qCritical("logo file not found: %s", qPrintable(filename));
  }
}

void AboutDialog::updateLogo() const
{
  if (m_pDeps->isDarkMode()) {
    qDebug("showing dark logo");
    setLogo(":/image/logo-dark.png");
  } else {
    qDebug("showing light logo");
    setLogo(":/image/logo-light.png");
  }
}

QString AboutDialog::importantDevelopers() const
{
  QStringList awesomePeople;
  awesomePeople

      // Chris is the ultimate creator, and the one who started it all in 2001.
      << "Chris Schoeneman"

      // Richard and Adam developed CosmoDeskflow, the 90's predecessor project.
      << "Richard Lee"
      << "Adam Feder"

      // Nick continued the legacy in 2009 started by Chris.
      << "Nick Bolton"

      // Volker wrote the first version of the GUI (QSynergy) in 2008.
      << "Volker Lanz"

      // Re-ignited the project in 2008 and rebuilt the community.
      << "Sorin Sbârnea"

      // Contributors of bug fixes in the early days.
      << "Ryan Breen"
      << "Guido Poschta"
      << "Bertrand Landry Hetu"
      << "Tom Chadwick"
      << "Brent Priddy"
      << "Jason Axelson"
      << "Jake Petroules"

      // Implemented Wayland support (libei and libportal).
      << "Peter Hutterer"
      << "Olivier Fourdan"

      // Symless employees (in order of joining).
      << "Kyle Bloom"
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

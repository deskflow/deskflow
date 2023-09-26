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

#include "OSXHelpers.h"
#include "AboutDialog.h"

AboutDialog::AboutDialog(MainWindow* parent, const AppConfig& config) :
    QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
    Ui::AboutDialogBase()
{
    setupUi(this);

    m_versionChecker.setApp(parent->appPath(config.synergycName()));
    QString version = m_versionChecker.getVersion();
#ifdef SYNERGY_REVISION
    version +=  '-';
    version += SYNERGY_REVISION;
#endif
    m_pLabelSynergyVersion->setText(version);

    QString buildDateString = QString::fromLocal8Bit(__DATE__).simplified();
    QDate buildDate = QLocale("en_US").toDate(buildDateString, "MMM d yyyy");
    m_pLabelBuildDate->setText(buildDate.toString(Qt::SystemLocaleLongDate));

    textEliteBackers->hide();
    labelEliteBackers->hide();
    labelEliteBackerLink->hide();
    labelCreditsLink->hide();
}

int AboutDialog::exec()
{
    //Sets the current build year into the copyright text
    label_3->setText(getCopyrights() + getKeyContributors());
    resizeWindow();
    updateLogo();

    return QDialog::exec();
}

void AboutDialog::resizeWindow()
{
    QSize size(600, 310);
    setMaximumSize(size);
    setMinimumSize(size);
    resize(size);
}

void AboutDialog::updateLogo() const
{
#if defined(Q_OS_MAC)
    if (isOSXInterfaceStyleDark()) {
        QPixmap logo(":/res/image/about-dark.png");
        if (!logo.isNull()) {
            label_Logo->setPixmap(logo);
        }
    }
#endif
}

QString AboutDialog::getKeyContributors() const
{
    return QString(R"(<p style="font-size: 14px">Key contributors<br>
                    <span style="font-size: 11px">Chris Schoeneman, Nick Bolton, Richard Lee, Adam Feder, Volker Lanz,
                    Ryan Breen, Guido Poschta, Bertrand Landry Hetu, Tom Chadwick, Brent Priddy, Kyle Bloom,
                    Daun Chung, Serhii Hadzhylov, Oleksandr Lysytsia, Olena Kutytska, Francisco Magalhães.</span>
                    </p>)");
}

QString AboutDialog::getCopyrights() const
{
    QString buildDateString = QString::fromLocal8Bit(__DATE__).simplified();
    QDate buildDate = QLocale("en_US").toDate(buildDateString, "MMM d yyyy");

    QString copyrights(R"(<p>Keyboard and mouse sharing application.<br /><br />Copyright © %%YEAR%% Symless Ltd.</p>)");
    return copyrights.replace(QString("%%YEAR%%"), QString::number(buildDate.year()));
}

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

#include <QtCore>
#include <QtGui>

#include "OSXHelpers.h"

AboutDialog::AboutDialog(QWidget* parent, const QString& synergyApp) :
	QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
	Ui::AboutDialogBase()
{
	setupUi(this);

	QString aboutText(R"(<p>
Keyboard and mouse sharing application. Cross platform and open source.<br /><br />
Copyright © 2012-%%YEAR%% Symless Ltd.<br />
Copyright © 2002-2012 Chris Schoeneman, Nick Bolton, Volker Lanz.<br /><br />
Synergy is released under the GNU General Public License (GPLv2).<br /><br />
Synergy is based on CosmoSynergy by Richard Lee and Adam Feder.<br />
The Synergy GUI is based on QSynergy by Volker Lanz.<br /><br />
Visit our website for help and info (symless.com).
</p>)");

	m_versionChecker.setApp(synergyApp);
	QString version = m_versionChecker.getVersion();
#ifdef SYNERGY_REVISION
    version +=  '-';
    version += SYNERGY_REVISION;
#endif
	m_pLabelSynergyVersion->setText(version);

	QString buildDateString = QString::fromLocal8Bit(__DATE__).simplified();
	QDate buildDate = QLocale("en_US").toDate(buildDateString, "MMM d yyyy");
	m_pLabelBuildDate->setText(buildDate.toString(Qt::SystemLocaleLongDate));

	//Sets the current build year into the copyright text
	label_3->setText(aboutText.replace(QString("%%YEAR%%"), QString::number(buildDate.year())));

	// change default size based on os
#if defined(Q_OS_MAC)
	QSize size(600, 380);
	setMaximumSize(size);
	setMinimumSize(size);
	resize(size);

    if (isOSXInterfaceStyleDark()) {
        QPixmap logo(":/res/image/about-dark.png");
        if (!logo.isNull()) {
            label_Logo->setPixmap(logo);
        }
    }

#elif defined(Q_OS_LINUX)
	QSize size(600, 330);
	setMaximumSize(size);
	setMinimumSize(size);
	resize(size);
#endif
}

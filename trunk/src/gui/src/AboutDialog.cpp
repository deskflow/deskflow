/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2008 Volker Lanz (vl@fidra.de)
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
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
#include <QtNetwork>

static QString getIPAddress()
{
	QList<QHostAddress> addresses = QNetworkInterface::allAddresses();

	for (int i = 0; i < addresses.size(); i++)
		if (addresses[i].protocol() == QAbstractSocket::IPv4Protocol && addresses[i] != QHostAddress(QHostAddress::LocalHost))
			return addresses[i].toString();

	return "Unknown";
}

AboutDialog::AboutDialog(QWidget* parent, const QString& synergyApp) :
	QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
	Ui::AboutDialogBase()
{
	setupUi(this);

	m_versionChecker.setApp(synergyApp);
	m_pLabelSynergyVersion->setText(m_versionChecker.getVersion());
	m_pLabelHostname->setText(QHostInfo::localHostName());
	m_pLabelIPAddress->setText(getIPAddress());
}


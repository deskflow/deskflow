/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2013 Bolton Software Ltd.
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

#include "QUtility.h"

void setIndexFromItemData(QComboBox* comboBox, const QVariant& itemData)
{
	for (int i = 0; i < comboBox->count(); ++i)
	{
		if (comboBox->itemData(i) == itemData)
		{
			comboBox->setCurrentIndex(i);
			return;
		}
	}
}

QString hash(const QString& string)
{
	QByteArray data = string.toUtf8();
	QByteArray hash = QCryptographicHash::hash(data, QCryptographicHash::Md5);
	return hash.toHex();
}

QString getFirstMacAddress()
{
	QString mac;
	foreach (const QNetworkInterface &interface,  QNetworkInterface::allInterfaces())
	{
		mac = interface.hardwareAddress();
		if (mac.size() != 0)
		{
			break;
		}
	}
	return mac;
}

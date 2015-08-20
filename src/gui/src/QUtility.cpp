/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2013 Synergy Si Ltd.
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
 * 
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the OpenSSL
 * library.
 * You must obey the GNU General Public License in all respects for all of
 * the code used other than OpenSSL. If you modify file(s) with this
 * exception, you may extend this exception to your version of the file(s),
 * but you are not obligated to do so. If you do not wish to do so, delete
 * this exception statement from your version. If you delete this exception
 * statement from all source files in the program, then also delete it here.
 */

#include "QUtility.h"

#include "ProcessorArch.h"

#if defined(Q_OS_LINUX)
#include <QProcess>
#endif

#if defined(Q_OS_WIN)
#define _WIN32_WINNT 0x0501
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

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

qProcessorArch getProcessorArch()
{
#if defined(Q_OS_WIN)
	SYSTEM_INFO systemInfo;
	GetNativeSystemInfo(&systemInfo);

	switch (systemInfo.wProcessorArchitecture) {
	case PROCESSOR_ARCHITECTURE_INTEL:
		return kProcessorArchWin32;
	case PROCESSOR_ARCHITECTURE_IA64:
		return kProcessorArchWin64;
	case PROCESSOR_ARCHITECTURE_AMD64:
		return kProcessorArchWin64;
	default:
		return kProcessorArchUnknown;
	}
#endif

#if defined(Q_OS_LINUX)
#ifdef __i386__
	return kProcessorArchLinux32;
#else
	return kProcessorArchLinux64;
#endif
#endif

	return kProcessorArchUnknown;
}

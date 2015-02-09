/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2013 Synergy Si Ltd.
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

#include "ProcessorArch.h"

#if defined(Q_OS_LINUX)
#include <QProcess>
#endif

#if defined(Q_OS_WIN)
#define _WIN32_WINNT 0x0501
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

#if defined(Q_OS_LINUX)
static const char kLinuxI686[] = "i686";
static const char kLinuxX8664[] = "x86_64";
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

int checkProcessorArch()
{
#if defined(Q_OS_WIN)
	SYSTEM_INFO systemInfo;
	GetNativeSystemInfo(&systemInfo);

	switch (systemInfo.wProcessorArchitecture) {
	case PROCESSOR_ARCHITECTURE_INTEL:
		return Win_x86;
	case PROCESSOR_ARCHITECTURE_IA64:
		return Win_x64;
	case PROCESSOR_ARCHITECTURE_AMD64:
		return Win_x64;
	default:
		return unknown;
	}
#elif defined(Q_OS_MAC)
	return Mac_i386;
#else
	QString program("uname");
	QStringList args("-m");
	QProcess process;
	process.setReadChannel(QProcess::StandardOutput);
	process.start(program, args);
	bool success = process.waitForStarted();

	QString out, error;
	if (success)
	{
		if (process.waitForFinished()) {
			out = process.readAllStandardOutput();
			error = process.readAllStandardError();
		}
	}

	out = out.trimmed();
	error = error.trimmed();

	if (out.isEmpty() ||
		!error.isEmpty() ||
		!success ||
		process.exitCode() != 0)
	{
		return unknown;
	}

	if (out == kLinuxI686) {
		return Linux_i686;
	}
	else if (out == kLinuxX8664) {
		return Linux_x86_64;
	}
#endif
	return unknown;
}

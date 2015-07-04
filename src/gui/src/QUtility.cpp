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
static const char kUbuntu[] = "Ubuntu";
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
	bool version32 = false;
	bool debPackaging = false;

	QString program1("uname");
	QStringList args1("-m");
	QProcess process1;
	process1.setReadChannel(QProcess::StandardOutput);
	process1.start(program1, args1);
	bool success = process1.waitForStarted();

	QString out, error;
	if (success)
	{
		if (process1.waitForFinished()) {
			out = process1.readAllStandardOutput();
			error = process1.readAllStandardError();
		}
	}

	out = out.trimmed();
	error = error.trimmed();

	if (out.isEmpty() ||
		!error.isEmpty() ||
		!success ||
		process1.exitCode() != 0)
	{
		return unknown;
	}

	if (out == kLinuxI686) {
		version32 = true;
	}

	QString program2("python");
	QStringList args2("-mplatform");
	QProcess process2;
	process2.setReadChannel(QProcess::StandardOutput);
	process2.start(program2, args2);
	success = process2.waitForStarted();

	if (success)
	{
		if (process2.waitForFinished()) {
			out = process2.readAllStandardOutput();
			error = process2.readAllStandardError();
		}
	}

	out = out.trimmed();
	error = error.trimmed();

	if (out.isEmpty() ||
		!error.isEmpty() ||
		!success ||
		process2.exitCode() != 0)
	{
		return unknown;
	}

	if (out.contains(kUbuntu)) {
		debPackaging = true;
	}

	if (version32) {
		if (debPackaging) {
			return Linux_deb_i686;
		}
		else {
			return Linux_rpm_i686;
		}
	}
	else {
		if (debPackaging) {
			return Linux_deb_x86_64;
		}
		else {
			return Linux_rpm_x86_64;
		}
	}
#endif
	return unknown;
}

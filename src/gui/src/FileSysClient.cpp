/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2015 Synergy Si, Std.
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

#include "FileSysClient.h"

#include "EditionType.h"
#include "QUtility.h"

#include <QDir>
#include <QProcess>
#include <QMessageBox>
#include <QCoreApplication>
#include <stdexcept>

void FileSysClient::queryPluginList()
{
	try {
		isDone(false);
		QString extension = "*" + Plugin::getOsSpecificExt();
		QStringList nameFilter(extension);

		QString installDir(m_CoreInterface.getInstalledDir()
							.append(QDir::separator())
							.append(Plugin::getOsSpecificInstallerLocation()));

		QString searchDirectory(installDir);
		QDir directory(searchDirectory);
		m_PluginList = directory.entryList(nameFilter);
		isDone(true);
	}
	catch (std::exception& e)
	{
		isDone(true);
		emit error(tr(	"An error occurred while trying to load the "
						"plugin list. Please contact the help desk, and "
						"provide the following details.\n\n%1").arg(e.what()));
	}
	emit queryPluginDone();
	return;
}

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

#define TRAY_RETRY_COUNT 10
#define TRAY_RETRY_WAIT 2000

#include "QSynergyApplication.h"
#include "MainWindow.h"

#include <QtCore>
#include <QtGui>

class QThreadImpl : public QThread
{
public:
	static void msleep(unsigned long msecs)
	{
		QThread::msleep(msecs);
	}
};

int waitForTray();

int main(int argc, char* argv[])
{
	QCoreApplication::setOrganizationName("Synergy");
	QCoreApplication::setOrganizationDomain("http://synergy-foss.org/");
	QCoreApplication::setApplicationName("Synergy");

	QSynergyApplication app(argc, argv);

#if !defined(Q_OS_MAC)
	if (!waitForTray())
	{
		return -1;
	}
	QApplication::setQuitOnLastWindowClosed(false);
#endif

	QTranslator qtTranslator;
	qtTranslator.load("qt_" + QLocale::system().name());
	app.installTranslator(&qtTranslator);

	QTranslator myappTranslator;
	myappTranslator.load("res/lang/QSynergy_" + QLocale::system().name());
	app.installTranslator(&myappTranslator);

	MainWindow mainWindow;
	mainWindow.start();

	return app.exec();
}

int waitForTray()
{
	// on linux, the system tray may not be available immediately after logging in,
	// so keep retrying but give up after a short time.
	int trayAttempts = 0;
	while (true)
	{
		if (QSystemTrayIcon::isSystemTrayAvailable())
		{
			break;
		}

		if (++trayAttempts > TRAY_RETRY_COUNT)
		{
			QMessageBox::critical(NULL, "Synergy",
				QObject::tr("System tray is unavailable, quitting."));
			return false;
		}

		QThreadImpl::msleep(TRAY_RETRY_WAIT);
	}
	return true;
}


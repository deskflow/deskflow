#include "QSynergyApplication.h"
#include "MainWindow.h"

#include <QtCore>
#include <QtGui>

int main(int argc, char* argv[])
{
	QCoreApplication::setOrganizationName("Fidra");
	QCoreApplication::setOrganizationDomain("www.fidra.de");
	QCoreApplication::setApplicationName("QSynergy");

	QSynergyApplication app(argc, argv);

#if !defined(Q_OS_MAC)
	if (!QSystemTrayIcon::isSystemTrayAvailable())
	{
		QMessageBox::critical(NULL, "QSynergy", QObject::tr("There doesn't seem to be a system tray available. Quitting."));
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

	MainWindow mainWindow(NULL);
	return app.exec();
}


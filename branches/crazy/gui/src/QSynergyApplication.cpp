#include "QSynergyApplication.h"
#include "MainWindow.h"

#include <QtCore>
#include <QtGui>

QSynergyApplication::QSynergyApplication(int& argc, char** argv) :
	QApplication(argc, argv)
{
}

void QSynergyApplication::commitData(QSessionManager&)
{
	foreach(QWidget* widget, topLevelWidgets())
	{
		MainWindow* mainWindow = qobject_cast<MainWindow*>(widget);
		if (mainWindow)
			mainWindow->saveSettings();
	}
}


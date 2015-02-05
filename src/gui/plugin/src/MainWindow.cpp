#include "MainWindow.h"
#include "ui_MainWindowBase.h"

MainWindow::MainWindow(Arguments& args, QWidget* parent) :
	QMainWindow(parent),
	m_Arguments(args)
{
	setupUi(this);
	appendInfo(m_Arguments.email);
}

MainWindow::~MainWindow()
{
}

void MainWindow::changeEvent(QEvent* e)
{
    QMainWindow::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
		retranslateUi(this);
        break;
    default:
        break;
    }
}

void MainWindow::appendInfo(QString& s)
{
	m_pTextEditInfo->append(s);
}

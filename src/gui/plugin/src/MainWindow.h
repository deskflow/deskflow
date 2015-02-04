#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "Arguments.h"

#include <QMainWindow>

#include "ui_MainWindowBase.h"

class MainWindow : public QMainWindow, public Ui::MainWindow {
    Q_OBJECT
public:
	MainWindow(Arguments& args, QWidget* parent = 0);
    ~MainWindow();

protected:
	void changeEvent(QEvent* e);

private:
	void appendInfo(QString& s);

private:
	Arguments m_Arguments;
};

#endif // MAINWINDOW_H

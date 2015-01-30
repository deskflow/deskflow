#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include <QMainWindow>

#include "ui_LoginWindowBase.h"

class LoginWindow : public QMainWindow, public Ui::LoginWindowBase
{
    Q_OBJECT
public:
    LoginWindow(QWidget *parent = 0);
    ~LoginWindow();

protected:
    void changeEvent(QEvent *e);

private:
    Ui::LoginWindow *ui;
};

#endif // LOGINWINDOW_H

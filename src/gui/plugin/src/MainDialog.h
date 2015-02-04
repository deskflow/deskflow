#ifndef MAINDIALOG_H
#define MAINDIALOG_H

#include "Arguments.h"

#include "ui_MainDialogBase.h"

#include <QDialog>

class MainDialog : public QDialog, public Ui::MainDialog {
    Q_OBJECT
public:
	MainDialog(Arguments& args, QWidget* parent = 0);
    ~MainDialog();

protected:
    void changeEvent(QEvent *e);
};

#endif // MAINDIALOG_H

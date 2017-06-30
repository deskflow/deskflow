#ifndef FAILEDLOGINDIALOG_H
#define FAILEDLOGINDIALOG_H

#include <QDialog>
#include <QString>

namespace Ui {
class FailedLoginDialog;
}

class FailedLoginDialog : public QDialog {
    Q_OBJECT

public:
    explicit FailedLoginDialog (QWidget* parent = 0, QString message = "");
    ~FailedLoginDialog ();

private:
    Ui::FailedLoginDialog* ui;
};

#endif // FAILEDLOGINDIALOG_H

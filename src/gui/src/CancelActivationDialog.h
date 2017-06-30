#ifndef CANCELACTIVATIONDIALOG_H
#define CANCELACTIVATIONDIALOG_H

#include <QDialog>

namespace Ui {
class CancelActivationDialog;
}

class CancelActivationDialog : public QDialog {
    Q_OBJECT

public:
    explicit CancelActivationDialog (QWidget* parent = 0);
    ~CancelActivationDialog ();

private:
    Ui::CancelActivationDialog* ui;
};

#endif // CANCELACTIVATIONDIALOG_H

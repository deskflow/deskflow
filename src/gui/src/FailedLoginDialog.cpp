#include "FailedLoginDialog.h"
#include "ui_FailedLoginDialog.h"

FailedLoginDialog::FailedLoginDialog (QWidget* parent, QString message)
    : QDialog (parent), ui (new Ui::FailedLoginDialog) {
    ui->setupUi (this);
    ui->messageLabel->setText (ui->messageLabel->text ().arg (message));
}

FailedLoginDialog::~FailedLoginDialog () {
    delete ui;
}

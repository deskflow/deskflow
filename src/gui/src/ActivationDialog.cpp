#include "ActivationDialog.h"
#include "ui_ActivationDialog.h"
#include "CancelActivationDialog.h"

ActivationDialog::ActivationDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::ActivationDialog)
{
	ui->setupUi(this);
}

ActivationDialog::~ActivationDialog()
{
	delete ui;
}

void ActivationDialog::reject()
{
	CancelActivationDialog cancelActivationDialog(this);
	if (QDialog::Accepted == cancelActivationDialog.exec()) {
		QDialog::reject();
	}
}

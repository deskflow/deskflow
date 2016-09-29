#include "ActivationDialog.h"
#include "ui_ActivationDialog.h"

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

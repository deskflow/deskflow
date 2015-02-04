#include "MainDialog.h"
#include "ui_MainDialogBase.h"

#include <QMovie>

MainDialog::MainDialog(Arguments& args, QWidget* parent) :
	QDialog(parent)
{
	setupUi(this);

	setFixedSize(size());

	QMovie *movie = new QMovie(":/res/image/spinning-wheel.gif");
	m_pLabelSpinning->setMovie(movie);
	movie->start();

	m_pLabelInfo->setText(args.email);
}

MainDialog::~MainDialog()
{
}

void MainDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
		retranslateUi(this);
        break;
    default:
        break;
    }
}

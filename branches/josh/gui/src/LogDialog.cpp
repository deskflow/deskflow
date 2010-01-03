#include "LogDialog.h"

#include <QProcess>

LogDialog::LogDialog (QWidget* parent, QProcess*& synergy) :
	QDialog (parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
	Ui::LogDialogBase(),
	m_pSynergy(synergy)
{
	setupUi(this);
}

void LogDialog::append(const QString& s)
{
	m_pLogOutput->append(s);
}

void LogDialog::readSynergyOutput()
{
	if (m_pSynergy)
	{
		QByteArray log;
		log += m_pSynergy->readAllStandardOutput();
		log += m_pSynergy->readAllStandardError();

		append(QString(log));
	}
}


#if !defined(LOGDIALOG_H)

#define LOGDIALOG_H

#include <QDialog>

#include "ui_LogDialogBase.h"

class QProcess;

class LogDialog : public QDialog, public Ui::LogDialogBase
{
	Q_OBJECT

	public:
		LogDialog(QWidget* parent, QProcess*& synergy);

	public:
		void append(const QString& s);

	public slots:
		void readSynergyOutput();

	private:
		QProcess*& m_pSynergy;
};

#endif

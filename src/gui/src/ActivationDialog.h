#ifndef ACTIVATIONDIALOG_H
#define ACTIVATIONDIALOG_H

#include <QDialog>

namespace Ui {
class ActivationDialog;
}

class AppConfig;

class ActivationDialog : public QDialog
{
	Q_OBJECT
	
public:
	explicit ActivationDialog(QWidget *parent, AppConfig& appConfig);
	~ActivationDialog();

public slots:
	void reject();
	void accept();

protected:
	void notifyActivation (QString identity);
	
private:
	Ui::ActivationDialog *ui;
	AppConfig* m_appConfig;
};

#endif // ACTIVATIONDIALOG_H

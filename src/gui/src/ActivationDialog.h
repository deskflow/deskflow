#ifndef ACTIVATIONDIALOG_H
#define ACTIVATIONDIALOG_H

#include <QDialog>
#include <SubscriptionManager.h>

namespace Ui {
class ActivationDialog;
}

class AppConfig;

class ActivationDialog : public QDialog
{
	Q_OBJECT
	
public:
	ActivationDialog(QWidget *parent, AppConfig& appConfig, 
					 SubscriptionManager& subscriptionManager);
	~ActivationDialog();

public slots:
	void reject();
	void accept();

protected:
	void notifyActivation (QString identity);
	
private:
	Ui::ActivationDialog *ui;
	AppConfig* m_appConfig;
	SubscriptionManager* m_subscriptionManager;
};

#endif // ACTIVATIONDIALOG_H

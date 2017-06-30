#ifndef ACTIVATIONDIALOG_H
#define ACTIVATIONDIALOG_H

#include <QDialog>
#include <LicenseManager.h>

namespace Ui {
class ActivationDialog;
}

class AppConfig;

class ActivationDialog : public QDialog {
    Q_OBJECT

public:
    ActivationDialog (QWidget* parent, AppConfig& appConfig,
                      LicenseManager& licenseManager);
    ~ActivationDialog ();

public slots:
    void reject ();
    void accept ();

protected:
    void refreshSerialKey ();

private:
    Ui::ActivationDialog* ui;
    AppConfig* m_appConfig;
    LicenseManager* m_LicenseManager;
};

#endif // ACTIVATIONDIALOG_H

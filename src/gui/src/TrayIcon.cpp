#include "TrayIcon.h"

void TrayIcon::tryCreate() 
{
    QSystemTrayIcon trayIcon;
    if (trayIcon.isSystemTrayAvailable()) {
        m_pTrayIcon->show();
        m_connector(m_pTrayIcon.get(), SIGNAL(activated(QSystemTrayIcon::ActivationReason)));
    }
    else {
        QTimer::singleShot(1500, this, &TrayIcon::tryCreate);
    }
}

void TrayIcon::set(const QIcon& icon) 
{
    m_pTrayIcon->setIcon(icon);
}

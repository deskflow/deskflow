#include "TrayIcon.h"
#include <iostream>

void TrayIcon::tryCreate() 
{
    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        std::cerr << "system tray available" << std::endl;
        m_pTrayIcon->show();
        m_connector(m_pTrayIcon.get(), SIGNAL(activated(QSystemTrayIcon::ActivationReason)));
    }
    else {
        std::cerr << "system tray available" << std::endl;
        QTimer::singleShot(1500, this, &TrayIcon::tryCreate);
    }
}

void TrayIcon::set(const QIcon& icon) 
{
    m_pTrayIcon->setIcon(icon);
}

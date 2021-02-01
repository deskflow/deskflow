#include "TrayIcon.h"

void TrayIcon::tryCreate() const
{
    QSystemTrayIcon trayIcon; // by creating a new tray icon, we actually make the DBus implementation refresh the connection (DBus)
    trayIcon.hide(); // we ony hide it in order for the compiler to not optimise-out the object (make some use of it)
    if (QSystemTrayIcon::isSystemTrayAvailable()) { // this ends up calling the underlying DBus connection (on DBus)
        m_pTrayIcon->show();
        m_connector(m_pTrayIcon.get(), SIGNAL(activated(QSystemTrayIcon::ActivationReason)));
    }
    else {
        QTimer::singleShot(2500, this, &TrayIcon::tryCreate);
    }
}

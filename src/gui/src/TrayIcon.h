#pragma once
#include <QObject>
#include <QTimer>
#include <QSystemTrayIcon>
#include <QMenu>
#include <functional>
#include <memory>

class TrayIcon : public QObject
{
    Q_OBJECT
public:
    using TConnector = std::function<void(QObject *, const char *)>;

    template<typename TActionIterator>
    void create(TActionIterator *begin, TActionIterator *end, TConnector connector) 
    {
        m_connector = connector;
        m_pTrayIconMenu = std::make_unique<QMenu>();

        for (TActionIterator *p {begin}; p != end; ++p) {
            if (*p) {
                m_pTrayIconMenu->addAction(*p);
            }
            else {
                m_pTrayIconMenu->addSeparator();
            }
        }

        m_pTrayIcon = std::make_unique<QSystemTrayIcon>();
        m_pTrayIcon->setContextMenu(m_pTrayIconMenu.get());
        m_pTrayIcon->setToolTip("Synergy");

        tryCreate();
    }

    void tryCreate();

    void set(const QIcon& icon);

private:
    std::unique_ptr<QSystemTrayIcon>    m_pTrayIcon {};
    std::unique_ptr<QMenu>              m_pTrayIconMenu {};
    TConnector                          m_connector;
};

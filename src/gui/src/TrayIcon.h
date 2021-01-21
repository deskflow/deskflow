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
    using TConnector = std::function<void(QObject const *, const char *)>;

    TrayIcon() 
    {
        m_set = [this](const QIcon &icon){
            m_init = [this,icon](){
                this->set(icon);
            };
        };
    }

    template<typename TActionContainer>
    void create(TActionContainer const &actionContainer, TConnector const &connector) 
    {
        m_connector = connector;
        m_pTrayIconMenu = std::make_unique<QMenu>();

        for (auto action: actionContainer) {
            if (action) {
                m_pTrayIconMenu->addAction(action);
            }
            else {
                m_pTrayIconMenu->addSeparator();
            }
        }

        m_pTrayIcon = std::make_unique<QSystemTrayIcon>();
        m_pTrayIcon->setContextMenu(m_pTrayIconMenu.get());
        m_pTrayIcon->setToolTip("Synergy");
        m_set = [this](const QIcon& icon) { m_pTrayIcon->setIcon(icon); };

        tryCreate();
        if (m_init) {
            m_init();
            m_init = std::function<void()>();
        }
    }

    void tryCreate() const;
    void set(const QIcon& icon) const { m_set(icon); }

private:
    std::unique_ptr<QSystemTrayIcon>        m_pTrayIcon {};
    std::unique_ptr<QMenu>                  m_pTrayIconMenu {};
    TConnector                              m_connector;
    std::function<void()>                   m_init;
    std::function<void(const QIcon& icon)>  m_set;
};

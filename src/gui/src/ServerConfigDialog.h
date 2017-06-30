/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2008 Volker Lanz (vl@fidra.de)
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#if !defined(SERVERCONFIGDIALOG__H)

#define SERVERCONFIGDIALOG__H

#include "ScreenSetupModel.h"
#include "ServerConfig.h"

#include "ui_ServerConfigDialogBase.h"

#include <QDialog>

class ServerConfigDialog : public QDialog, public Ui::ServerConfigDialogBase {
    Q_OBJECT

public:
    ServerConfigDialog (QWidget* parent, ServerConfig& config,
                        const QString& defaultScreenName);

public slots:
    void accept ();
    void showEvent (QShowEvent* event);
    void
    message (const QString& message) {
        m_Message = message;
    }

protected slots:
    void on_m_pButtonNewHotkey_clicked ();
    void on_m_pListHotkeys_itemSelectionChanged ();
    void on_m_pButtonEditHotkey_clicked ();
    void on_m_pButtonRemoveHotkey_clicked ();

    void on_m_pButtonNewAction_clicked ();
    void on_m_pListActions_itemSelectionChanged ();
    void on_m_pButtonEditAction_clicked ();
    void on_m_pButtonRemoveAction_clicked ();

protected:
    ServerConfig&
    serverConfig () {
        return m_ServerConfig;
    }
    void
    setOrigServerConfig (const ServerConfig& s) {
        m_OrigServerConfig = s;
    }
    ScreenSetupModel&
    model () {
        return m_ScreenSetupModel;
    }

private:
    ServerConfig& m_OrigServerConfig;
    ServerConfig m_ServerConfig;
    ScreenSetupModel m_ScreenSetupModel;
    QString m_Message;
};

#endif

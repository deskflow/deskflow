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

#include "ScreenSettingsDialog.h"
#include "ServerConfigDialog.h"
#include "ServerConfig.h"
#include "HotkeyDialog.h"
#include "ActionDialog.h"
#include "UpgradeDialog.h"

#include <QtCore>
#include <QtGui>
#include <QMessageBox>
#include <QFileDialog>

ServerConfigDialog::ServerConfigDialog(QWidget* parent, ServerConfig& config) :
    QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
    Ui::ServerConfigDialogBase(),
    m_OrigServerConfig(config),
    m_OrigServerAppConfigUseExternalConfig(config.getUseExternalConfig()),
    m_OrigServerAppConfigExternalConfigFile(config.getConfigFile()),
    m_ServerConfig(config),
    m_ScreenSetupModel(serverConfig().screens(), serverConfig().numColumns(), serverConfig().numRows()),
    m_Message("")
{
    setupUi(this);

    m_pEditConfigFile->setText(serverConfig().getConfigFile());
    m_pCheckBoxUseExternalConfig->setChecked(serverConfig().getUseExternalConfig());
    m_pCheckBoxHeartbeat->setChecked(serverConfig().hasHeartbeat());
    m_pSpinBoxHeartbeat->setValue(serverConfig().heartbeat());

    m_pCheckBoxRelativeMouseMoves->setChecked(serverConfig().relativeMouseMoves());
    m_pCheckBoxWin32KeepForeground->setChecked(serverConfig().win32KeepForeground());

    m_pCheckBoxSwitchDelay->setChecked(serverConfig().hasSwitchDelay());
    m_pSpinBoxSwitchDelay->setValue(serverConfig().switchDelay());

    m_pCheckBoxSwitchDoubleTap->setChecked(serverConfig().hasSwitchDoubleTap());
    m_pSpinBoxSwitchDoubleTap->setValue(serverConfig().switchDoubleTap());

    m_pCheckBoxCornerTopLeft->setChecked(serverConfig().switchCorner(BaseConfig::TopLeft));
    m_pCheckBoxCornerTopRight->setChecked(serverConfig().switchCorner(BaseConfig::TopRight));
    m_pCheckBoxCornerBottomLeft->setChecked(serverConfig().switchCorner(BaseConfig::BottomLeft));
    m_pCheckBoxCornerBottomRight->setChecked(serverConfig().switchCorner(BaseConfig::BottomRight));
    m_pSpinBoxSwitchCornerSize->setValue(serverConfig().switchCornerSize());
    m_pCheckBoxDisableLockToScreen->setChecked(serverConfig().disableLockToScreen());

    m_pCheckBoxIgnoreAutoConfigClient->setChecked(serverConfig().ignoreAutoConfigClient());

    m_pCheckBoxEnableClipboard->setChecked(serverConfig().clipboardSharing());
	int clipboardSharingSizeM = static_cast<int>(serverConfig().clipboardSharingSize() / 1024);
    m_pSpinBoxClipboardSizeLimit->setValue(clipboardSharingSizeM);
	m_pSpinBoxClipboardSizeLimit->setEnabled(serverConfig().clipboardSharing());

    foreach(const Hotkey& hotkey, serverConfig().hotkeys())
        m_pListHotkeys->addItem(hotkey.text());

    m_pScreenSetupView->setModel(&m_ScreenSetupModel);

    auto& screens = serverConfig().screens();
    auto server = std::find_if(screens.begin(), screens.end(), [this](const Screen& screen){ return (screen.name() == serverConfig().getServerName());});

    if (server == screens.end()) {
        Screen serverScreen(serverConfig().getServerName());
        serverScreen.markAsServer();
        model().screen(serverConfig().numColumns() / 2, serverConfig().numRows() / 2) = serverScreen;
    }
    else {
        server->markAsServer();
    }

    m_pButtonAddComputer->setEnabled(!model().isFull());
    connect(m_pTrashScreenWidget, SIGNAL(screenRemoved()), this, SLOT(onScreenRemoved()));

    onChange();

    //computers
    connect(&m_ScreenSetupModel, &ScreenSetupModel::screensChanged, this, &ServerConfigDialog::onChange);

    //advanced
    connect(m_pCheckBoxSwitchDelay,                                            &QCheckBox::stateChanged,
            this, [this]( const int& v ) { serverConfig().haveSwitchDelay(v);                          onChange();});
    connect(m_pSpinBoxSwitchDelay,        static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, [this]( const int& v ) { serverConfig().setSwitchDelay(v);                           onChange();});
    connect(m_pCheckBoxSwitchDoubleTap,                                        &QCheckBox::stateChanged,
            this, [this]( const int& v ) { serverConfig().haveSwitchDoubleTap(v);                      onChange();});
    connect(m_pSpinBoxSwitchDoubleTap,    static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, [this]( const int& v ) { serverConfig().setSwitchDoubleTap(v);                       onChange();});
    connect(m_pCheckBoxEnableClipboard,                                        &QCheckBox::stateChanged,
            this, [this]( const int& v ) { serverConfig().setClipboardSharing(v);                      onChange();});
    connect(m_pSpinBoxClipboardSizeLimit, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, [this]( const int& v ) { serverConfig().setClipboardSharingSize(v * 1024);           onChange();});
    connect(m_pCheckBoxHeartbeat,                                              &QCheckBox::stateChanged,
            this, [this]( const int& v ) { serverConfig().haveHeartbeat(v);                            onChange();});
    connect(m_pSpinBoxHeartbeat,          static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, [this]( const int& v ) { serverConfig().setHeartbeat(v);                             onChange();});
    connect(m_pCheckBoxRelativeMouseMoves,                                     &QCheckBox::stateChanged,
            this, [this]( const int& v ) { serverConfig().setRelativeMouseMoves(v);                    onChange();});
    connect(m_pCheckBoxWin32KeepForeground,                                    &QCheckBox::stateChanged,
            this, [this]( const int& v ) { serverConfig().setWin32KeepForeground(v);                   onChange();});
    connect(m_pCheckBoxIgnoreAutoConfigClient,                                 &QCheckBox::stateChanged,
            this, [this]( const int& v ) { serverConfig().setIgnoreAutoConfigClient(v);                onChange();});
    connect(m_pCheckBoxDisableLockToScreen,                                    &QCheckBox::stateChanged,
            this, [this]( const int& v ) { serverConfig().setDisableLockToScreen(v);                   onChange();});
    connect(m_pCheckBoxCornerTopLeft,                                          &QCheckBox::stateChanged,
            this, [this]( const int& v ) { serverConfig().setSwitchCorner(BaseConfig::TopLeft, v);     onChange();});
    connect(m_pCheckBoxCornerTopRight,                                         &QCheckBox::stateChanged,
            this, [this]( const int& v ) { serverConfig().setSwitchCorner(BaseConfig::TopRight, v);    onChange();});
    connect(m_pCheckBoxCornerBottomLeft,                                       &QCheckBox::stateChanged,
            this, [this]( const int& v ) { serverConfig().setSwitchCorner(BaseConfig::BottomLeft, v);  onChange();});
    connect(m_pCheckBoxCornerBottomRight,                                      &QCheckBox::stateChanged,
            this, [this]( const int& v ) { serverConfig().setSwitchCorner(BaseConfig::BottomRight, v); onChange();});
    connect(m_pSpinBoxSwitchCornerSize,   static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, [this]( const int& v ) { serverConfig().setSwitchCornerSize(v);                      onChange();});

    //config
    connect(m_pCheckBoxUseExternalConfig, &QCheckBox::stateChanged,
            this, [this]( const int& v ) { serverConfig().setUseExternalConfig(v); onChange();});
    connect(m_pEditConfigFile,            &QLineEdit::textChanged,
            this, [this]() { serverConfig().setConfigFile(m_pEditConfigFile->text()); onChange();});
}

bool ServerConfigDialog::addClient(const QString& clientName)
{
    return addComputer(clientName, true);
}

void ServerConfigDialog::showEvent(QShowEvent* event)
{
    QDialog::show();

    if (!m_Message.isEmpty())
    {
        // TODO: ideally this massage box should pop up after the dialog is shown
        QMessageBox::information(this, tr("Configure server"), m_Message);
    }
}

void ServerConfigDialog::accept()
{
    if (m_pCheckBoxUseExternalConfig->isChecked() &&
        !QFile::exists(m_pEditConfigFile->text()))
    {
        auto title = tr("Configuration filename invalid");
        auto description = tr("You have not filled in a valid configuration file for the synergy server. "
                              "Do you want to browse for the configuration file now?");

        auto selectedButton = QMessageBox::warning(this, title, description, QMessageBox::Yes | QMessageBox::No);

        if (selectedButton != QMessageBox::Yes ||
            !on_m_pButtonBrowseConfigFile_clicked())
        {
           return;
        }
    }

    // now that the dialog has been accepted, copy the new server config to the original one,
    // which is a reference to the one in MainWindow.
    setOrigServerConfig(serverConfig());

    QDialog::accept();
}

void ServerConfigDialog::reject()
{
    serverConfig().setUseExternalConfig(m_OrigServerAppConfigUseExternalConfig);
    serverConfig().setConfigFile(m_OrigServerAppConfigExternalConfigFile);

    QDialog::reject();
}

void ServerConfigDialog::on_m_pButtonNewHotkey_clicked()
{
    if (serverConfig().isHotkeysAvailable()) {
        Hotkey hotkey;
        HotkeyDialog dlg(this, hotkey);
        if (dlg.exec() == QDialog::Accepted) {
            serverConfig().hotkeys().append(hotkey);
            m_pListHotkeys->addItem(hotkey.text());
            onChange();
        }
    }
    else {
        UpgradeDialog upgradeDialog(this);
        upgradeDialog.showDialog(
            "Configuring custom hotkeys is a Synergy Ultimate feature.",
            "synergy/purchase/purchase-ultimate-upgrade?source=gui"
        );
    }
}

void ServerConfigDialog::on_m_pButtonEditHotkey_clicked()
{
    int idx = m_pListHotkeys->currentRow();
    Q_ASSERT(idx >= 0 && idx < serverConfig().hotkeys().size());
    Hotkey& hotkey = serverConfig().hotkeys()[idx];
    HotkeyDialog dlg(this, hotkey);
    if (dlg.exec() == QDialog::Accepted) {
        m_pListHotkeys->currentItem()->setText(hotkey.text());
        onChange();
    }
}

void ServerConfigDialog::on_m_pButtonRemoveHotkey_clicked()
{
    int idx = m_pListHotkeys->currentRow();
    Q_ASSERT(idx >= 0 && idx < serverConfig().hotkeys().size());
    serverConfig().hotkeys().removeAt(idx);
    m_pListActions->clear();
    delete m_pListHotkeys->item(idx);
    onChange();
}

void ServerConfigDialog::on_m_pListHotkeys_itemSelectionChanged()
{
    bool itemsSelected = !m_pListHotkeys->selectedItems().isEmpty();
    m_pButtonEditHotkey->setEnabled(itemsSelected);
    m_pButtonRemoveHotkey->setEnabled(itemsSelected);
    m_pButtonNewAction->setEnabled(itemsSelected);

    if (itemsSelected && serverConfig().hotkeys().size() > 0)
    {
        m_pListActions->clear();

        int idx = m_pListHotkeys->row(m_pListHotkeys->selectedItems()[0]);

        // There's a bug somewhere around here: We get idx == 1 right after we deleted the next to last item, so idx can
        // only possibly be 0. GDB shows we got called indirectly from the delete line in
        // on_m_pButtonRemoveHotkey_clicked() above, but the delete is of course necessary and seems correct.
        // The while() is a generalized workaround for all that and shouldn't be required.
        while (idx >= 0 && idx >= serverConfig().hotkeys().size())
            idx--;

        Q_ASSERT(idx >= 0 && idx < serverConfig().hotkeys().size());

        const Hotkey& hotkey = serverConfig().hotkeys()[idx];
        foreach(const Action& action, hotkey.actions())
            m_pListActions->addItem(action.text());
    }
}

void ServerConfigDialog::on_m_pButtonNewAction_clicked()
{
    int idx = m_pListHotkeys->currentRow();
    Q_ASSERT(idx >= 0 && idx < serverConfig().hotkeys().size());
    Hotkey& hotkey = serverConfig().hotkeys()[idx];

    Action action;
    ActionDialog dlg(this, serverConfig(), hotkey, action);
    if (dlg.exec() == QDialog::Accepted)
    {
        hotkey.actions().append(action);
        m_pListActions->addItem(action.text());
        onChange();
    }
}

void ServerConfigDialog::on_m_pButtonEditAction_clicked()
{
    int idxHotkey = m_pListHotkeys->currentRow();
    Q_ASSERT(idxHotkey >= 0 && idxHotkey < serverConfig().hotkeys().size());
    Hotkey& hotkey = serverConfig().hotkeys()[idxHotkey];

    int idxAction = m_pListActions->currentRow();
    Q_ASSERT(idxAction >= 0 && idxAction < hotkey.actions().size());
    Action& action = hotkey.actions()[idxAction];

    ActionDialog dlg(this, serverConfig(), hotkey, action);
    if (dlg.exec() == QDialog::Accepted) {
        m_pListActions->currentItem()->setText(action.text());
        onChange();
    }
}

void ServerConfigDialog::on_m_pButtonRemoveAction_clicked()
{
    int idxHotkey = m_pListHotkeys->currentRow();
    Q_ASSERT(idxHotkey >= 0 && idxHotkey < serverConfig().hotkeys().size());
    Hotkey& hotkey = serverConfig().hotkeys()[idxHotkey];

    int idxAction = m_pListActions->currentRow();
    Q_ASSERT(idxAction >= 0 && idxAction < hotkey.actions().size());

    hotkey.actions().removeAt(idxAction);
    delete m_pListActions->currentItem();
    onChange();
}

void ServerConfigDialog::on_m_pCheckBoxEnableClipboard_stateChanged(int const state)
{
    m_pSpinBoxClipboardSizeLimit->setEnabled (state == Qt::Checked);
    if ((state == Qt::Checked) && (!m_pSpinBoxClipboardSizeLimit->value())) {
        int size = static_cast<int>((serverConfig().defaultClipboardSharingSize() + 512) / 1024);
        m_pSpinBoxClipboardSizeLimit->setValue (size ? size : 1);
    }
}

void ServerConfigDialog::on_m_pListActions_itemSelectionChanged()
{
    m_pButtonEditAction->setEnabled(!m_pListActions->selectedItems().isEmpty());
    m_pButtonRemoveAction->setEnabled(!m_pListActions->selectedItems().isEmpty());
}

void ServerConfigDialog::on_m_pButtonAddComputer_clicked()
{
    addComputer("", false);
}

void ServerConfigDialog::onScreenRemoved()
{
    m_pButtonAddComputer->setEnabled(true);
    onChange();
}

void ServerConfigDialog::on_m_pCheckBoxUseExternalConfig_toggled(bool checked)
{
    m_pLabelConfigFile->setEnabled(checked);
    m_pEditConfigFile->setEnabled(checked);
    m_pButtonBrowseConfigFile->setEnabled(checked);

    m_pTabWidget->setTabEnabled(0, !checked);
    m_pTabWidget->setTabEnabled(1, !checked);
    m_pTabWidget->setTabEnabled(2, !checked);
}

bool ServerConfigDialog::on_m_pButtonBrowseConfigFile_clicked()
{
#if defined(Q_OS_WIN)
    const QString synergyConfigFilter(QObject::tr("Synergy Configurations (*.sgc);;All files (*.*)"));
#else
    const QString synergyConfigFilter(QObject::tr("Synergy Configurations (*.conf);;All files (*.*)"));
#endif

    QString fileName = QFileDialog::getOpenFileName(this, tr("Browse for a synergys config file"), QString(), synergyConfigFilter);

    if (!fileName.isEmpty())
    {
        m_pEditConfigFile->setText(fileName);
        return true;
    }

    return false;
}

bool ServerConfigDialog::addComputer(const QString& clientName, bool doSilent)
{
    bool isAccepted = false;
    Screen newScreen(clientName);


    ScreenSettingsDialog dlg(this, &newScreen, &model().m_Screens);
    if (doSilent || dlg.exec() == QDialog::Accepted)
    {
        model().addScreen(newScreen);
        isAccepted = true;
    }

    m_pButtonAddComputer->setEnabled(!model().isFull());
    return isAccepted;
}

void ServerConfigDialog::onChange()
{
    bool isAppConfigDataEqual = m_OrigServerAppConfigUseExternalConfig  == serverConfig().getUseExternalConfig() &&
                                m_OrigServerAppConfigExternalConfigFile == serverConfig().getConfigFile();
    m_pButtonBox->button(QDialogButtonBox::Ok)->setEnabled(!isAppConfigDataEqual || !(m_OrigServerConfig == m_ServerConfig));
}

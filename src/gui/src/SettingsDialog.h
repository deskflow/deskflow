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

#if !defined(SETTINGSDIALOG_H)

#define SETTINGSDIALOG_H

#include <QDialog>
#include "ui_SettingsDialogBase.h"
#include "SynergyLocale.h"
#include "CoreInterface.h"

class MainWindow;
class AppConfig;
class BonjourWindows;

class SettingsDialog : public QDialog, public Ui::SettingsDialogBase
{
    Q_OBJECT

    public:
        SettingsDialog(QWidget* parent, AppConfig& config);
        static QString browseForSynergyc(QWidget* parent, const QString& programDir, const QString& synergycName);
        static QString browseForSynergys(QWidget* parent, const QString& programDir, const QString& synergysName);
        void allowAutoConfig();

    protected:
        void accept();
        void reject();
        void changeEvent(QEvent* event);
        AppConfig& appConfig() { return m_appConfig; }

        /// @brief Causes the dialog to load all the settings from m_appConfig
        void loadFromConfig();

        /// @brief Check if the regenerate button should be enabled or disabled and sets it
        void updateRegenButton();

        /// @brief Updates the key length value based on the loaded file
        /// @param [in] QString path The path to the file to test
        void updateKeyLengthOnFile(const QString& path);

    private:
        MainWindow* m_pMainWindow;
        AppConfig& m_appConfig;
        SynergyLocale m_Locale;
        CoreInterface m_CoreInterface;
        BonjourWindows* m_pBonjourWindows;

    private slots:
        void on_m_pCheckBoxEnableCrypto_toggled(bool checked);
        void on_m_pComboLanguage_currentIndexChanged(int index);
        void on_m_pCheckBoxLogToFile_stateChanged(int );
        void on_m_pButtonBrowseLog_clicked();
        void on_m_pLabelInstallBonjour_linkActivated(const QString &link);

        /// @brief Handles the toggling of the system scoped radio button
        ///        As the user scope radio is connected this will fire for either radio button
        void on_m_pRadioSystemScope_toggled(bool checked);

        /// @brief Handles the click event of the Cert Path browse button
        ///        displaying a file browser
        void on_m_pPushButtonBrowseCert_clicked();

        /// @brief Handles the TLS cert key length changed event
        void on_m_pComboBoxKeyLength_currentIndexChanged(int index);

        /// @brief handels the regenerate cert button event
        ///         This will regenerate the TLS certificate as long as the settings haven't changed
        void on_m_pPushButtonRegenCert_clicked();
};

#endif

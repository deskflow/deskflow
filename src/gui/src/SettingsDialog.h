/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Synergy Si Ltd.
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
 * 
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the OpenSSL
 * library.
 * You must obey the GNU General Public License in all respects for all of
 * the code used other than OpenSSL. If you modify file(s) with this
 * exception, you may extend this exception to your version of the file(s),
 * but you are not obligated to do so. If you do not wish to do so, delete
 * this exception statement from your version. If you delete this exception
 * statement from all source files in the program, then also delete it here.
 */

#if !defined(SETTINGSDIALOG_H)

#define SETTINGSDIALOG_H

#include <QDialog>
#include "ui_SettingsDialogBase.h"
#include "SynergyLocale.h"
#include "CoreInterface.h"

class AppConfig;

class SettingsDialog : public QDialog, public Ui::SettingsDialogBase
{
	Q_OBJECT

	public:
		SettingsDialog(QWidget* parent, AppConfig& config);
		static QString browseForSynergyc(QWidget* parent, const QString& programDir, const QString& synergycName);
		static QString browseForSynergys(QWidget* parent, const QString& programDir, const QString& synergysName);

	protected:
		void accept();
		void reject();
		void changeEvent(QEvent* event);
		AppConfig& appConfig() { return m_AppConfig; }

	private:
		AppConfig& m_AppConfig;
		SynergyLocale m_Locale;
		CoreInterface m_CoreInterface;
		bool m_SuppressElevateWarning;

	private slots:
		void on_m_pCheckBoxEnableCrypto_toggled(bool checked);
		void on_m_pCheckBoxElevateMode_toggled(bool checked);
		void on_m_pComboLanguage_currentIndexChanged(int index);
		void on_m_pCheckBoxLogToFile_stateChanged(int );
		void on_m_pButtonBrowseLog_clicked();
};

#endif

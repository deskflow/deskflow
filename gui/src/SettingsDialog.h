#if !defined(SETTINGSDIALOG_H)

#define SETTINGSDIALOG_H

#include <QDialog>
#include "ui_SettingsDialogBase.h"

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
		AppConfig& appConfig() { return m_AppConfig; }

	private:
		AppConfig& m_AppConfig;

	private slots:
		void on_m_pCheckBoxLogToFile_stateChanged(int );
		bool on_m_pButtonBrowseSynergys_clicked();
		bool on_m_pButtonBrowseSynergyc_clicked();
		void on_m_pCheckBoxAutoDetectPaths_stateChanged(int i);
		void on_m_pButtonBrowseLog_clicked();
};

#endif

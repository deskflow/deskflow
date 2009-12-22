#if !defined(SERVERCONFIGDIALOG__H)

#define SERVERCONFIGDIALOG__H

#include "ScreenSetupModel.h"
#include "ServerConfig.h"

#include "ui_ServerConfigDialogBase.h"

#include <QDialog>

class ServerConfigDialog : public QDialog, public Ui::ServerConfigDialogBase
{
	Q_OBJECT

	public:
		ServerConfigDialog(QWidget* parent, ServerConfig& config, const QString& defaultScreenName);

	public slots:
		void accept();

	protected slots:
		void on_m_pButtonNewHotkey_clicked();
		void on_m_pListHotkeys_itemSelectionChanged();
		void on_m_pButtonEditHotkey_clicked();
		void on_m_pButtonRemoveHotkey_clicked();

		void on_m_pButtonNewAction_clicked();
		void on_m_pListActions_itemSelectionChanged();
		void on_m_pButtonEditAction_clicked();
		void on_m_pButtonRemoveAction_clicked();

	protected:
		ServerConfig& serverConfig() { return m_ServerConfig; }
		void setOrigServerConfig(const ServerConfig& s) { m_OrigServerConfig = s; }
		ScreenSetupModel& model() { return m_ScreenSetupModel; }

	private:
		ServerConfig& m_OrigServerConfig;
		ServerConfig m_ServerConfig;
		ScreenSetupModel m_ScreenSetupModel;
};

#endif


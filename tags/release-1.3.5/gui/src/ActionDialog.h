#if !defined(ACTIONDIALOG_H)

#define ACTIONDIALOG_H

#include <QDialog>

#include "ui_ActionDialogBase.h"

class Hotkey;
class Action;
class QRadioButton;
class QButtonGroup;
class ServerConfig;

class ActionDialog : public QDialog, public Ui::ActionDialogBase
{
	Q_OBJECT

	public:
		ActionDialog(QWidget* parent, ServerConfig& config, Hotkey& hotkey, Action& action);

	protected slots:
		void accept();
		void on_m_pKeySequenceWidgetHotkey_keySequenceChanged();

	protected:
		const KeySequenceWidget* sequenceWidget() const { return m_pKeySequenceWidgetHotkey; }
		const ServerConfig& serverConfig() const { return m_ServerConfig; }

	private:
		const ServerConfig& m_ServerConfig;
		Hotkey& m_Hotkey;
		Action& m_Action;

		QButtonGroup* m_pButtonGroupType;
};

#endif

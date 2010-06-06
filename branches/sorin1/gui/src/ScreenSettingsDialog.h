#if !defined(SCREENSETTINGSDIALOG__H)

#define SCREENSETTINGSDIALOG__H

#include <QDialog>

#include "ui_ScreenSettingsDialogBase.h"

class QWidget;
class QString;

class Screen;

class ScreenSettingsDialog : public QDialog, public Ui::ScreenSettingsDialogBase
{
	Q_OBJECT

	public:
		ScreenSettingsDialog(QWidget* parent, Screen* pScreen = NULL);

	public slots:
		void accept();

	private slots:
		void on_m_pButtonAddAlias_clicked();
		void on_m_pButtonRemoveAlias_clicked();
		void on_m_pLineEditAlias_textChanged(const QString& text);
		void on_m_pListAliases_itemSelectionChanged();

	private:
		Screen* m_pScreen;
};

#endif


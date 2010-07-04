#if !defined(ABOUTDIALOG__H)

#define ABOUTDIALOG__H

#include <QDialog>

#include "ui_AboutDialogBase.h"

class QWidget;
class QString;

class AboutDialog : public QDialog, public Ui::AboutDialogBase
{
	Q_OBJECT

	public:
		AboutDialog(QWidget* parent, const QString& synergyApp = QString());
};

#endif


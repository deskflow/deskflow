#if !defined(SCREENSETUPVIEW__H)

#define SCREENSETUPVIEW__H

#include <QTableView>
#include <QFlags>

#include "Screen.h"

class QWidget;
class QMouseEvent;
class QResizeEvent;
class QDragEnterEvent;
class ScreenSetupModel;

class ScreenSetupView : public QTableView
{
	Q_OBJECT

	public:
		ScreenSetupView(QWidget* parent);

	public:
		void setModel(ScreenSetupModel* model);
		ScreenSetupModel* model() const;

	protected:
		void mouseDoubleClickEvent(QMouseEvent*);
		void setTableSize();
		void resizeEvent(QResizeEvent*);
		void dragEnterEvent(QDragEnterEvent* event);
		void dragMoveEvent(QDragMoveEvent* event);
		void startDrag(Qt::DropActions supportedActions);
		QStyleOptionViewItem viewOptions() const;
		void scrollTo(const QModelIndex&, ScrollHint) {}
};

#endif


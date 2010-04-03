#if !defined(TRASHSCREENWIDGET__H)

#define TRASHSCREENWIDGET__H

#include <QLabel>

class QWidget;
class QDragEnterEvent;
class QDropEvent;

class TrashScreenWidget : public QLabel
{
	Q_OBJECT

	public:
		TrashScreenWidget(QWidget* parent) : QLabel(parent) {}

	public:
		void dragEnterEvent(QDragEnterEvent* event);
		void dropEvent(QDropEvent* event);
};

#endif


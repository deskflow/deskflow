#if !defined(NEWSCREENWIDGET__H)

#define NEWSCREENWIDGET__H

#include <QLabel>

class QMouseEvent;
class QWidget;

class NewScreenWidget : public QLabel
{
	Q_OBJECT

	public:
		NewScreenWidget(QWidget* parent);

	protected:
		void mousePressEvent(QMouseEvent* event);
};

#endif


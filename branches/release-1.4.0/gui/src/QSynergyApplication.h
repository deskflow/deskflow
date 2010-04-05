#if !defined(QSYNERGYAPPLICATION__H)

#define QSYNERGYAPPLICATION__H

#include <QApplication>

class QSessionManager;

class QSynergyApplication : public QApplication
{
	public:
		QSynergyApplication(int& argc, char** argv);

	public:
		void commitData(QSessionManager& manager);
};

#endif


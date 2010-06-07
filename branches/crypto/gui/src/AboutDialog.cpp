#include "AboutDialog.h"

#include <QtCore>
#include <QtGui>
#include <QtNetwork>

static QString getSynergyVersion(const QString& app)
{
#if !defined(Q_OS_WIN)
	QProcess process;
	process.start(app, QStringList() << "--version");

	process.setReadChannel(QProcess::StandardError);
	if (!process.waitForStarted() || !process.waitForFinished())
		return QObject::tr("(unknown)");

	QRegExp rx("synergy[cs] ([\\d\\.]+)");
	if (rx.indexIn(QString(process.readLine())) != -1)
		return rx.cap(1);
#else
	Q_UNUSED(app);
#endif

	return QObject::tr("(unknown)");
}

static QString getIPAddress()
{
	QList<QHostAddress> addresses = QNetworkInterface::allAddresses();

	for (int i = 0; i < addresses.size(); i++)
		if (addresses[i].protocol() == QAbstractSocket::IPv4Protocol && addresses[i] != QHostAddress(QHostAddress::LocalHost))
			return addresses[i].toString();

	return QObject::tr("(unknown)");
}

AboutDialog::AboutDialog(QWidget* parent, const QString& synergyApp) :
	QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
	Ui::AboutDialogBase()
{
	setupUi(this);

	m_pLabelSynergyVersion->setText(getSynergyVersion(synergyApp));
	m_pLabelHostname->setText(QHostInfo::localHostName());
	m_pLabelIPAddress->setText(getIPAddress());
}


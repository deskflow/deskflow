#ifndef SSLCERTIFICATE_H
#define SSLCERTIFICATE_H

#include "CoreInterface.h"

#include <QObject>

class SslCertificate : public QObject
{
Q_OBJECT

public:
	explicit SslCertificate(QObject *parent = 0);

public slots:
	void generateCertificate();

signals:
	void error(QString e);
	void info(QString i);
	void generateCertificateFinished();

private:
	bool checkOpenSslBinary();
	bool runProgram(
		const QString& program,
		const QStringList& args,
		const QStringList& env);

private:
	QString m_ProfileDir;
	QString m_standardOutput;
	CoreInterface m_CoreInterface;
};

#endif // SSLCERTIFICATE_H

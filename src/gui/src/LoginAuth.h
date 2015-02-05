#ifndef LOGINAUTH_H
#define LOGINAUTH_H

#include <QString>
#include <QObject>

class LoginWindow;

enum qUserType {
	Student,
	Home,
	Professional,
	Error,
	ExceptionError,
	InvalidEmailPassword,
	ServerResponseError,
	Unknown

};

class LoginAuth : public QObject
{
	Q_OBJECT

public:
	int doCheckUserType();
	void setEmail(QString email) { m_Email = email; }
	void setPassword(QString password) { m_Password = password; }
	void setLoginWindow(LoginWindow* w) { m_pLoginWindow = w; }

public slots:
	void checkUserType();

signals:
	void finished();

private:
	QString request(const QString& email, const QString& password);
	QString hash(const QString& string);

private:
	QString m_Email;
	QString m_Password;
	LoginWindow* m_pLoginWindow;
};

#endif // LOGINAUTH_H

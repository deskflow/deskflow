/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2015 Synergy Si Ltd.
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LOGINAUTH_H
#define LOGINAUTH_H

#include <QString>
#include <QObject>

class LoginWindow;
class AppConfig;

class LoginAuth : public QObject
{
	Q_OBJECT

public:
	int doCheckUserType(int& edition);
	void setEmail(QString email) { m_Email = email; }
	void setPassword(QString password) { m_Password = password; }
	void setLoginWindow(LoginWindow* w) { m_pLoginWindow = w; }

public slots:
	void checkUserType();

signals:
	void finished();

private:
	QString request(const QString& email, const QString& password);

private:
	QString m_Email;
	QString m_Password;
	LoginWindow* m_pLoginWindow;
};

#endif // LOGINAUTH_H

/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2015 Synergy Si, Std.
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

#ifndef WEBCLIENT_H
#define WEBCLIENT_H

#include <QString>
#include <QObject>

class QMessageBox;
class QWidget;

class WebClient : public QObject
{
	Q_OBJECT

public:
	int getEdition(const QString& email,
			const QString& password,
			QMessageBox& message,
			QWidget* w);

private:
	QString request(const QString& email, const QString& password);
};

#endif // WEBCLIENT_H

/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2014 Synergy Si, Inc.
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef COMMANDTHREAD_H
#define COMMANDTHREAD_H

#include <QStringList>

class CommandProcess : public QObject
{
	Q_OBJECT

public:
	CommandProcess(QString cmd, QStringList arguments);

signals:
	void finished();

public slots:
	void run();

private:
	QString m_Command;
	QStringList m_Arguments;
};

#endif // COMMANDTHREAD_H

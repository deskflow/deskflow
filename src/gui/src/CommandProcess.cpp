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

#include "CommandProcess.h"

#include <QProcess>

CommandProcess::CommandProcess(QString cmd, QStringList arguments) :
	m_Command(cmd),
	m_Arguments(arguments)
{
}

void CommandProcess::run()
{
	QProcess process;
	process.start(m_Command, m_Arguments);
	process.waitForFinished();
	emit finished();
}

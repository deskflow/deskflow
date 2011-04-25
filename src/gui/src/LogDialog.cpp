/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2008 Volker Lanz (vl@fidra.de)
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

#include "LogDialog.h"

#include <QProcess>

LogDialog::LogDialog (QWidget* parent, QProcess*& synergy) :
	QDialog (parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
	Ui::LogDialogBase(),
	m_pSynergy(synergy)
{
	setupUi(this);
}

void LogDialog::append(const QString& s)
{
	m_pLogOutput->append(s);
}

void LogDialog::readSynergyOutput()
{
	if (m_pSynergy)
	{
		QByteArray log;
		log += m_pSynergy->readAllStandardOutput();
		log += m_pSynergy->readAllStandardError();

		append(QString(log));
	}
}


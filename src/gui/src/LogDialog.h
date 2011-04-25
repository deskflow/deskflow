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

#if !defined(LOGDIALOG_H)

#define LOGDIALOG_H

#include <QDialog>

#include "ui_LogDialogBase.h"

class QProcess;

class LogDialog : public QDialog, public Ui::LogDialogBase
{
	Q_OBJECT

	public:
		LogDialog(QWidget* parent, QProcess*& synergy);

	public:
		void append(const QString& s);
		void clear() { m_pLogOutput->clear(); }

	public slots:
		void readSynergyOutput();

	private:
		QProcess*& m_pSynergy;
};

#endif

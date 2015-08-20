/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Synergy Si Ltd.
 * Copyright (C) 2008 Volker Lanz (vl@fidra.de)
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
 * 
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the OpenSSL
 * library.
 * You must obey the GNU General Public License in all respects for all of
 * the code used other than OpenSSL. If you modify file(s) with this
 * exception, you may extend this exception to your version of the file(s),
 * but you are not obligated to do so. If you do not wish to do so, delete
 * this exception statement from your version. If you delete this exception
 * statement from all source files in the program, then also delete it here.
 */

#if !defined(QSYNERGYAPPLICATION__H)

#define QSYNERGYAPPLICATION__H

#include <QApplication>

class QSessionManager;

class QSynergyApplication : public QApplication
{
	public:
		QSynergyApplication(int& argc, char** argv);
		~QSynergyApplication();

	public:
		void commitData(QSessionManager& manager);
		void switchTranslator(QString lang);
		void setTranslator(QTranslator* translator);

		static QSynergyApplication* getInstance();

	private:
		QTranslator* m_Translator;

		static QSynergyApplication* s_Instance;
};

#endif


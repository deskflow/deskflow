/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2021 Symless Ltd.
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
 */

#ifndef SERVERSTATELABEL_H
#define SERVERSTATELABEL_H

#include <QLabel>

namespace synergy_widgets
{

class ServerStateLabel : public QLabel
{
public:
   explicit ServerStateLabel(QWidget* parent = nullptr);
   void updateServerState(const QString& line);

private:
   unsigned int m_clientsCounter = 0;

   void updateState();
};

} //namespace synergy_widgets

#endif // SERVERSTATELABEL_H

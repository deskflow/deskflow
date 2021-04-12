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

#include "ServerStateLabel.h"

namespace synergy_widgets
{

ServerStateLabel::ServerStateLabel(QWidget* parent) :
   QLabel(parent)
{
}

void ServerStateLabel::updateServerState(const QString& line)
{
   if (line.contains("process exited"))
   {
      m_clientsCounter = 0;
   }
   else if (line.contains("has connected"))
   {
      ++m_clientsCounter;
   }
   else if (line.contains("has disconnected") && m_clientsCounter)
   {
      --m_clientsCounter;
   }

   updateState();
}

void ServerStateLabel::updateState()
{
   if (m_clientsCounter)
   {
      hide();
   }
   else
   {
      show();
   }
}

} //namespace synergy_widgets

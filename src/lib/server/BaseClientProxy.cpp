/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Synergy Si Ltd.
 * Copyright (C) 2006 Chris Schoeneman
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

#include "server/BaseClientProxy.h"

//
// BaseClientProxy
//

BaseClientProxy::BaseClientProxy(const String& name) :
	m_name(name),
	m_x(0),
	m_y(0)
{
	// do nothing
}

BaseClientProxy::~BaseClientProxy()
{
	// do nothing
}

void
BaseClientProxy::setJumpCursorPos(SInt32 x, SInt32 y)
{
	m_x = x;
	m_y = y;
}

void
BaseClientProxy::getJumpCursorPos(SInt32& x, SInt32& y) const
{
	x = m_x;
	y = m_y;
}

String
BaseClientProxy::getName() const
{
	return m_name;
}

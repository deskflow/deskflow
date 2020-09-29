/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2015 Synergy Seamless Inc.
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

#include "SerialKeyType.h"

const std::string SerialKeyType::TRIAL = "trial";
const std::string SerialKeyType::SUBSCRIPTION = "subscription";

SerialKeyType::SerialKeyType()
{

}

void
SerialKeyType::setKeyType(const std::string& Type)
{
	m_isTrial = false;
	m_isTemporary = false;

	if (Type == SerialKeyType::TRIAL){
		m_isTrial = true;
		m_isTemporary = true;
	}
	else if (Type == SerialKeyType::SUBSCRIPTION){
		m_isTemporary = true;
	}
}

bool
SerialKeyType::isTrial() const
{
	return m_isTrial;
}

bool
SerialKeyType::isTemporary() const
{
	return m_isTemporary;
}

bool
SerialKeyType::isPermanent() const
{
	return (!m_isTemporary);
}


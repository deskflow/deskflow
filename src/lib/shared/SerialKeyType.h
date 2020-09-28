/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2016 Symless Ltd.
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
#pragma once

#include <string>

class SerialKeyType
{
private:
	friend bool operator== (SerialKeyType const&, SerialKeyType const&);
public:
	static const std::string TRIAL;
	static const std::string SUBSCRIPTION;

	SerialKeyType();

	void setKeyType(const std::string& Type);
	bool isTrial() const;
	bool isTemporary() const;
	bool isPermanent() const;

private:
	bool m_isTrial = false;
	bool m_isTemporary = false;
};

inline bool
operator== (SerialKeyType const& lhs, SerialKeyType const& rhs) {
	return (lhs.m_isTrial == rhs.m_isTrial) && (lhs.m_isTemporary == rhs.m_isTemporary);
}

inline bool
operator!= (SerialKeyType const& lhs, SerialKeyType const& rhs) {
	return !(lhs == rhs);
}


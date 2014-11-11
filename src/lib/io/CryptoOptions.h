/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2013 Bolton Software Ltd.
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

#pragma once

#include "base/String.h"
#include "io/ECryptoMode.h"

//! Encapsulates basic crypto options
class CryptoOptions {
public:
	CryptoOptions() : m_mode(kDisabled) { }
	CryptoOptions(const String& modeString, const String& pass);
	
	//! Return enum for mode string
	static ECryptoMode	parseMode(String modeString);

	//! Parse and set mode string
	void				setMode(String modeString);

	String				m_pass;
	ECryptoMode			m_mode;
	String				m_modeString;
};

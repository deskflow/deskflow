/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2014 Bolton Software Ltd.
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

#include "synergy/App.h"
#include "arch/Arch.h"
#include "base/Log.h"

NS1(synergy)

class CDeploymentApp : public CMinimalApp {
public:
	CDeploymentApp();
	virtual ~CDeploymentApp();

	int					run(int argc, char** argv);

private:
	CArch				m_arch;
	CLog				m_log;
};

_NS1

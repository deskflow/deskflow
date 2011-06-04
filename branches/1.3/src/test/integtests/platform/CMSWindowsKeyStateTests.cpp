/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2011 Chris Schoeneman, Nick Bolton, Sorin Sbarnea
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

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "CMSWindowsKeyState.h"
#include "CMSWindowsDesks.h"
#include "CMSWindowsScreen.h"
#include "CMSWindowsScreenSaver.h"
#include "TMethodJob.h"
#include "CMockEventQueue.h"
#include "CMockKeyMap.h"

using ::testing::_;
using ::testing::NiceMock;

class CMSWindowsKeyStateTests : public ::testing::Test
{
protected:
	virtual void SetUp()
	{
		// load synrgyhk.dll
		m_hookLibrary = m_hookLibraryLoader.openHookLibrary("synrgyhk");
		m_screensaver = new CMSWindowsScreenSaver();
		m_desks = new CMSWindowsDesks(
			true, m_hookLibrary, m_screensaver,
			new TMethodJob<CMSWindowsKeyStateTests>(
				this, &CMSWindowsKeyStateTests::updateKeysCB));
	}

	virtual void TearDown()
	{
		delete m_screensaver;
		delete m_desks;
	}

	CMSWindowsDesks* getDesks() const
	{
		return m_desks;
	}

	void* getEventTarget() const
	{
		return const_cast<CMSWindowsKeyStateTests*>(this);
	}

private:
	void updateKeysCB(void*) { }
	HINSTANCE m_hookLibrary;
	IScreenSaver* m_screensaver;
	CMSWindowsDesks* m_desks;
	CMSWindowsHookLibraryLoader m_hookLibraryLoader;
};

TEST_F(CMSWindowsKeyStateTests, disable_nonWin95OS_eventQueueNotUsed)
{
	NiceMock<CMockEventQueue> eventQueue;
	CMockKeyMap keyMap;
	CMSWindowsKeyState keyState(getDesks(), getEventTarget(), eventQueue, keyMap);
	
	// in anything above win95-family, event handler should not be called.
	EXPECT_CALL(eventQueue, removeHandler(_, _)).Times(0);

	keyState.disable();
}

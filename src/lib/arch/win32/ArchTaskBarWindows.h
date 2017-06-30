/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2003 Chris Schoeneman
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

#include "arch/IArchTaskBar.h"
#include "arch/IArchMultithread.h"
#include "common/stdmap.h"
#include "common/stdvector.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#define ARCH_TASKBAR ArchTaskBarWindows

//! Win32 implementation of IArchTaskBar
class ArchTaskBarWindows : public IArchTaskBar {
public:
    ArchTaskBarWindows ();
    virtual ~ArchTaskBarWindows ();

    virtual void init ();

    //! Add a dialog window
    /*!
    Tell the task bar event loop about a dialog.  Win32 annoyingly
    requires messages destined for modeless dialog boxes to be
    dispatched differently than other messages.
    */
    static void addDialog (HWND);

    //! Remove a dialog window
    /*!
    Remove a dialog window added via \c addDialog().
    */
    static void removeDialog (HWND);

    // IArchTaskBar overrides
    virtual void addReceiver (IArchTaskBarReceiver*);
    virtual void removeReceiver (IArchTaskBarReceiver*);
    virtual void updateReceiver (IArchTaskBarReceiver*);

private:
    class ReceiverInfo {
    public:
        UINT m_id;
    };

    typedef std::map<IArchTaskBarReceiver*, ReceiverInfo> ReceiverToInfoMap;
    typedef std::map<UINT, ReceiverToInfoMap::iterator> CIDToReceiverMap;
    typedef std::vector<UINT> CIDStack;
    typedef std::map<HWND, bool> Dialogs;

    UINT getNextID ();
    void recycleID (UINT);

    void addIcon (UINT);
    void removeIcon (UINT);
    void updateIcon (UINT);
    void addAllIcons ();
    void removeAllIcons ();
    void
    modifyIconNoLock (ReceiverToInfoMap::const_iterator, DWORD taskBarMessage);
    void removeIconNoLock (UINT id);
    void handleIconMessage (IArchTaskBarReceiver*, LPARAM);

    bool processDialogs (MSG*);
    LRESULT wndProc (HWND, UINT, WPARAM, LPARAM);
    static LRESULT CALLBACK staticWndProc (HWND, UINT, WPARAM, LPARAM);
    void threadMainLoop ();
    static void* threadEntry (void*);

    HINSTANCE instanceWin32 ();

private:
    static ArchTaskBarWindows* s_instance;

    // multithread data
    ArchMutex m_mutex;
    ArchCond m_condVar;
    bool m_ready;
    int m_result;
    ArchThread m_thread;

    // child thread data
    HWND m_hwnd;
    UINT m_taskBarRestart;

    // shared data
    ReceiverToInfoMap m_receivers;
    CIDToReceiverMap m_idTable;
    CIDStack m_oldIDs;
    UINT m_nextID;

    // dialogs
    Dialogs m_dialogs;
    Dialogs m_addedDialogs;
};

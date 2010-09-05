/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2003 Chris Schoeneman
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "CArchTaskBarWindows.h"
#include "CArchMiscWindows.h"
#include "IArchTaskBarReceiver.h"
#include "CArch.h"
#include "XArch.h"
#include <string.h>
#include <shellapi.h>

static const UINT		kAddReceiver     = WM_USER + 10;
static const UINT		kRemoveReceiver  = WM_USER + 11;
static const UINT		kUpdateReceiver  = WM_USER + 12;
static const UINT		kNotifyReceiver  = WM_USER + 13;
static const UINT		kFirstReceiverID = WM_USER + 14;

//
// CArchTaskBarWindows
//

CArchTaskBarWindows*	CArchTaskBarWindows::s_instance    = NULL;
HINSTANCE				CArchTaskBarWindows::s_appInstance = NULL;

CArchTaskBarWindows::CArchTaskBarWindows(void* appInstance) :
	m_nextID(kFirstReceiverID)
{
	// save the singleton instance
	s_instance    = this;

	// save app instance
	s_appInstance = reinterpret_cast<HINSTANCE>(appInstance);

	// we need a mutex
	m_mutex       = ARCH->newMutex();

	// and a condition variable which uses the above mutex
	m_ready       = false;
	m_condVar     = ARCH->newCondVar();

	// we're going to want to get a result from the thread we're
	// about to create to know if it initialized successfully.
	// so we lock the condition variable.
	ARCH->lockMutex(m_mutex);

	// open a window and run an event loop in a separate thread.
	// this has to happen in a separate thread because if we
	// create a window on the current desktop with the current
	// thread then the current thread won't be able to switch
	// desktops if it needs to.
	m_thread      = ARCH->newThread(&CArchTaskBarWindows::threadEntry, this);

	// wait for child thread
	while (!m_ready) {
		ARCH->waitCondVar(m_condVar, m_mutex, -1.0);
	}

	// ready
	ARCH->unlockMutex(m_mutex);
}

CArchTaskBarWindows::~CArchTaskBarWindows()
{
	if (m_thread != NULL) {
		PostMessage(m_hwnd, WM_QUIT, 0, 0);
		ARCH->wait(m_thread, -1.0);
		ARCH->closeThread(m_thread);
	}
	ARCH->closeCondVar(m_condVar);
	ARCH->closeMutex(m_mutex);
	s_instance = NULL;
}

void
CArchTaskBarWindows::addDialog(HWND hwnd)
{
	CArchMiscWindows::addDialog(hwnd);
}

void
CArchTaskBarWindows::removeDialog(HWND hwnd)
{
	CArchMiscWindows::removeDialog(hwnd);
}

void
CArchTaskBarWindows::addReceiver(IArchTaskBarReceiver* receiver)
{
	// ignore bogus receiver
	if (receiver == NULL) {
		return;
	}

	// add receiver if necessary
	CReceiverToInfoMap::iterator index = m_receivers.find(receiver);
	if (index == m_receivers.end()) {
		// add it, creating a new message ID for it
		CReceiverInfo info;
		info.m_id = getNextID();
		index = m_receivers.insert(std::make_pair(receiver, info)).first;

		// add ID to receiver mapping
		m_idTable.insert(std::make_pair(info.m_id, index));
	}

	// add receiver
	PostMessage(m_hwnd, kAddReceiver, index->second.m_id, 0);
}

void
CArchTaskBarWindows::removeReceiver(IArchTaskBarReceiver* receiver)
{
	// find receiver
	CReceiverToInfoMap::iterator index = m_receivers.find(receiver);
	if (index == m_receivers.end()) {
		return;
	}

	// remove icon.  wait for this to finish before returning.
	SendMessage(m_hwnd, kRemoveReceiver, index->second.m_id, 0);

	// recycle the ID
	recycleID(index->second.m_id);

	// discard
	m_idTable.erase(index->second.m_id);
	m_receivers.erase(index);
}

void
CArchTaskBarWindows::updateReceiver(IArchTaskBarReceiver* receiver)
{
	// find receiver
	CReceiverToInfoMap::const_iterator index = m_receivers.find(receiver);
	if (index == m_receivers.end()) {
		return;
	}

	// update icon and tool tip
	PostMessage(m_hwnd, kUpdateReceiver, index->second.m_id, 0);
}

UINT
CArchTaskBarWindows::getNextID()
{
	if (m_oldIDs.empty()) {
		return m_nextID++;
	}
	UINT id = m_oldIDs.back();
	m_oldIDs.pop_back();
	return id;
}

void
CArchTaskBarWindows::recycleID(UINT id)
{
	m_oldIDs.push_back(id);
}

void
CArchTaskBarWindows::addIcon(UINT id)
{
	ARCH->lockMutex(m_mutex);
	CIDToReceiverMap::const_iterator index = m_idTable.find(id);
	if (index != m_idTable.end()) {
		modifyIconNoLock(index->second, NIM_ADD);
	}
	ARCH->unlockMutex(m_mutex);
}

void
CArchTaskBarWindows::removeIcon(UINT id)
{
	ARCH->lockMutex(m_mutex);
	removeIconNoLock(id);
	ARCH->unlockMutex(m_mutex);
}

void
CArchTaskBarWindows::updateIcon(UINT id)
{
	ARCH->lockMutex(m_mutex);
	CIDToReceiverMap::const_iterator index = m_idTable.find(id);
	if (index != m_idTable.end()) {
		modifyIconNoLock(index->second, NIM_MODIFY);
	}
	ARCH->unlockMutex(m_mutex);
}

void
CArchTaskBarWindows::addAllIcons()
{
	ARCH->lockMutex(m_mutex);
	for (CReceiverToInfoMap::const_iterator index = m_receivers.begin();
									index != m_receivers.end(); ++index) {
		modifyIconNoLock(index, NIM_ADD);
	}
	ARCH->unlockMutex(m_mutex);
}

void
CArchTaskBarWindows::removeAllIcons()
{
	ARCH->lockMutex(m_mutex);
	for (CReceiverToInfoMap::const_iterator index = m_receivers.begin();
									index != m_receivers.end(); ++index) {
		removeIconNoLock(index->second.m_id);
	}
	ARCH->unlockMutex(m_mutex);
}

void
CArchTaskBarWindows::modifyIconNoLock(
				CReceiverToInfoMap::const_iterator index, DWORD taskBarMessage)
{
	// get receiver
	UINT id                        = index->second.m_id;
	IArchTaskBarReceiver* receiver = index->first;

	// lock receiver so icon and tool tip are guaranteed to be consistent
	receiver->lock();

	// get icon data
	HICON icon = reinterpret_cast<HICON>(
				const_cast<IArchTaskBarReceiver::Icon>(receiver->getIcon()));

	// get tool tip
	std::string toolTip = receiver->getToolTip();

	// done querying
	receiver->unlock();

	// prepare to add icon
	NOTIFYICONDATA data;
	data.cbSize           = sizeof(NOTIFYICONDATA);
	data.hWnd             = m_hwnd;
	data.uID              = id;
	data.uFlags           = NIF_MESSAGE;
	data.uCallbackMessage = kNotifyReceiver;
	data.hIcon            = icon;
	if (icon != NULL) {
		data.uFlags |= NIF_ICON;
	}
	if (!toolTip.empty()) {
		strncpy(data.szTip, toolTip.c_str(), sizeof(data.szTip));
		data.szTip[sizeof(data.szTip) - 1] = '\0';
		data.uFlags                       |= NIF_TIP;
	}
	else {
		data.szTip[0] = '\0';
	}

	// add icon
	if (Shell_NotifyIcon(taskBarMessage, &data) == 0) {
		// failed
	}
}

void
CArchTaskBarWindows::removeIconNoLock(UINT id)
{
	NOTIFYICONDATA data;
	data.cbSize = sizeof(NOTIFYICONDATA);
	data.hWnd   = m_hwnd;
	data.uID    = id;
	if (Shell_NotifyIcon(NIM_DELETE, &data) == 0) {
		// failed
	}
}

void
CArchTaskBarWindows::handleIconMessage(
				IArchTaskBarReceiver* receiver, LPARAM lParam)
{
	// process message
	switch (lParam) {
	case WM_LBUTTONDOWN:
		receiver->showStatus();
		break;

	case WM_LBUTTONDBLCLK:
		receiver->primaryAction();
		break;

	case WM_RBUTTONUP: {
		POINT p;
		GetCursorPos(&p);
		receiver->runMenu(p.x, p.y);
		break;
	}

	case WM_MOUSEMOVE:
		// currently unused
		break;

	default:
		// unused
		break;
	}
}

bool
CArchTaskBarWindows::processDialogs(MSG* msg)
{
	// only one thread can be in this method on any particular object
	// at any given time.  that's not a problem since only our event
	// loop calls this method and there's just one of those.

	ARCH->lockMutex(m_mutex);

	// remove removed dialogs
	m_dialogs.erase(false);

	// merge added dialogs into the dialog list
	for (CDialogs::const_iterator index = m_addedDialogs.begin();
							index != m_addedDialogs.end(); ++index) {
		m_dialogs.insert(std::make_pair(index->first, index->second));
	}
	m_addedDialogs.clear();

	ARCH->unlockMutex(m_mutex);

	// check message against all dialogs until one handles it.
	// note that we don't hold a lock while checking because
	// the message is processed and may make calls to this
	// object.  that's okay because addDialog() and
	// removeDialog() don't change the map itself (just the
	// values of some elements).
	ARCH->lockMutex(m_mutex);
	for (CDialogs::const_iterator index = m_dialogs.begin();
							index != m_dialogs.end(); ++index) {
		if (index->second) {
			ARCH->unlockMutex(m_mutex);
			if (IsDialogMessage(index->first, msg)) {
				return true;
			}
			ARCH->lockMutex(m_mutex);
		}
	}
	ARCH->unlockMutex(m_mutex);

	return false;
}

LRESULT
CArchTaskBarWindows::wndProc(HWND hwnd,
				UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case kNotifyReceiver: {
		// lookup receiver
		CIDToReceiverMap::const_iterator index = m_idTable.find(wParam);
		if (index != m_idTable.end()) {
			IArchTaskBarReceiver* receiver = index->second->first;
			handleIconMessage(receiver, lParam);
			return 0;
		}
		break;
	}

	case kAddReceiver:
		addIcon(wParam);
		break;

	case kRemoveReceiver:
		removeIcon(wParam);
		break;

	case kUpdateReceiver:
		updateIcon(wParam);
		break;

	default:
		if (msg == m_taskBarRestart) {
			// task bar was recreated so re-add our icons
			addAllIcons();
		}
		break;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK
CArchTaskBarWindows::staticWndProc(HWND hwnd, UINT msg,
				WPARAM wParam, LPARAM lParam)
{
	// if msg is WM_NCCREATE, extract the CArchTaskBarWindows* and put
	// it in the extra window data then forward the call.
	CArchTaskBarWindows* self = NULL;
	if (msg == WM_NCCREATE) {
		CREATESTRUCT* createInfo;
		createInfo = reinterpret_cast<CREATESTRUCT*>(lParam);
		self       = reinterpret_cast<CArchTaskBarWindows*>(
												createInfo->lpCreateParams);
		SetWindowLong(hwnd, 0, reinterpret_cast<LONG>(self));
	}
	else {
		// get the extra window data and forward the call
		LONG data = GetWindowLong(hwnd, 0);
		if (data != 0) {
			self = reinterpret_cast<CArchTaskBarWindows*>(
							reinterpret_cast<void*>(data));
		}
	}

	// forward the message
	if (self != NULL) {
		return self->wndProc(hwnd, msg, wParam, lParam);
	}
	else {
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
}

void
CArchTaskBarWindows::threadMainLoop()
{
	// register the task bar restart message
	m_taskBarRestart        = RegisterWindowMessage(TEXT("TaskbarCreated"));

	// register a window class
	WNDCLASSEX classInfo;
	classInfo.cbSize        = sizeof(classInfo);
	classInfo.style         = CS_NOCLOSE;
	classInfo.lpfnWndProc   = &CArchTaskBarWindows::staticWndProc;
	classInfo.cbClsExtra    = 0;
	classInfo.cbWndExtra    = sizeof(CArchTaskBarWindows*);
	classInfo.hInstance     = s_appInstance;
	classInfo.hIcon         = NULL;
	classInfo.hCursor       = NULL;
	classInfo.hbrBackground = NULL;
	classInfo.lpszMenuName  = NULL;
	classInfo.lpszClassName = TEXT("SynergyTaskBar");
	classInfo.hIconSm       = NULL;
	ATOM windowClass        = RegisterClassEx(&classInfo);

	// create window
	m_hwnd = CreateWindowEx(WS_EX_TOOLWINDOW,
							reinterpret_cast<LPCTSTR>(windowClass),
							TEXT("Synergy Task Bar"),
							WS_POPUP,
							0, 0, 1, 1,
							NULL,
							NULL,
							s_appInstance,
							reinterpret_cast<void*>(this));

	// signal ready
	ARCH->lockMutex(m_mutex);
	m_ready = true;
	ARCH->broadcastCondVar(m_condVar);
	ARCH->unlockMutex(m_mutex);

	// handle failure
	if (m_hwnd == NULL) {
		UnregisterClass(reinterpret_cast<LPCTSTR>(windowClass), s_appInstance);
		return;
	}

	// main loop
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		if (!processDialogs(&msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	// clean up
	removeAllIcons();
	DestroyWindow(m_hwnd);
	UnregisterClass(reinterpret_cast<LPCTSTR>(windowClass), s_appInstance);
}

void*
CArchTaskBarWindows::threadEntry(void* self)
{
	reinterpret_cast<CArchTaskBarWindows*>(self)->threadMainLoop();
	return NULL;
}

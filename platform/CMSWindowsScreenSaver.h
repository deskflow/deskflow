#ifndef CMSWINDOWSSCREENSAVER_H
#define CMSWINDOWSSCREENSAVER_H

#include "IScreenSaver.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

class CThread;

class CMSWindowsScreenSaver : public IScreenSaver {
public:
	CMSWindowsScreenSaver();
	virtual ~CMSWindowsScreenSaver();

	// manipulators

	// check if the screen saver really started.  returns false if it
	// hasn't, true otherwise.  when the screen saver stops msg will
	// be posted to the current thread's message queue with the given
	// parameters.
	bool				checkStarted(UINT msg, WPARAM, LPARAM);

	// IScreenSaver overrides
	virtual void		enable();
	virtual void		disable();
	virtual void		activate();
	virtual void		deactivate();
	virtual bool		isActive() const;

private:
	class CFindScreenSaverInfo {
	public:
		HDESK			m_desktop;
		HWND			m_window;
	};

	static BOOL CALLBACK	findScreenSaverFunc(HWND hwnd, LPARAM lParam);
	static BOOL CALLBACK	killScreenSaverFunc(HWND hwnd, LPARAM lParam);

	HWND				findScreenSaver();
	void				watchProcess(HANDLE process);
	void				unwatchProcess();
	void				watchProcessThread(void*);

private:
	bool				m_is95Family;
	bool				m_is95;
	bool				m_isNT;
	BOOL				m_wasEnabled;

	HANDLE				m_process;
	CThread*			m_watch;
	DWORD				m_threadID;
	UINT				m_msg;
	WPARAM				m_wParam;
	LPARAM				m_lParam;
};

#endif

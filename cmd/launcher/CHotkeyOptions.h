/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2006 Chris Schoeneman
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

#ifndef CHOTKEYOPTIONS_H
#define CHOTKEYOPTIONS_H

#include "CString.h"
#include "KeyTypes.h"
#include "MouseTypes.h"
#include "CInputFilter.h"

#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>

class CConfig;

//! Hotkey options dialog for Microsoft Windows launcher
class CHotkeyOptions {
public:
	CHotkeyOptions(HWND parent, CConfig*);
	~CHotkeyOptions();

	//! @name manipulators
	//@{

	//! Run dialog
	/*!
	Display and handle the dialog until closed by the user.
	*/
	void				doModal();

	//@}
	//! @name accessors
	//@{

	//@}

private:
	void				doInit(HWND hwnd);

	void				fillHotkeys(HWND hwnd, UInt32 select = (UInt32)-1);
	void				updateHotkeysControls(HWND hwnd);

	void				addHotkey(HWND hwnd);
	void				removeHotkey(HWND hwnd);
	void				editHotkey(HWND hwnd);

	void				fillActions(HWND hwnd, UInt32 select = (UInt32)-1);
	void				updateActionsControls(HWND hwnd);

	void				addAction(HWND hwnd);
	void				removeAction(HWND hwnd);
	void				editAction(HWND hwnd);

	bool				editCondition(HWND hwnd, CInputFilter::CCondition*&);
	bool				editAction(HWND hwnd, CInputFilter::CAction*&,
							bool& onActivate);

	void				openRule(HWND hwnd);
	void				closeRule(HWND hwnd);
	UInt32				findMatchingAction(
							const CInputFilter::CKeystrokeAction*) const;
	UInt32				findMatchingAction(
							const CInputFilter::CMouseButtonAction*) const;

	// message handling
	BOOL				doDlgProc(HWND, UINT, WPARAM, LPARAM);
	static BOOL CALLBACK dlgProc(HWND, UINT, WPARAM, LPARAM);

	// special actions we use to combine matching down/up actions into a
	// single action for the convenience of the user.
	class CKeystrokeDownUpAction : public CInputFilter::CKeystrokeAction {
	public:
		CKeystrokeDownUpAction(IPlatformScreen::CKeyInfo* adoptedInfo) :
			CInputFilter::CKeystrokeAction(adoptedInfo, true) { }

		// CAction overrides
		virtual CInputFilter::CAction*	clone() const
		{
			IKeyState::CKeyInfo* info = IKeyState::CKeyInfo::alloc(*getInfo());
			return new CKeystrokeDownUpAction(info);
		}

	protected:
		// CKeystrokeAction overrides
		virtual const char*		formatName() const { return "keystroke"; }
	};
	class CMouseButtonDownUpAction : public CInputFilter::CMouseButtonAction {
	public:
		CMouseButtonDownUpAction(IPrimaryScreen::CButtonInfo* adoptedInfo) :
			CInputFilter::CMouseButtonAction(adoptedInfo, true) { }

		// CAction overrides
		virtual CInputFilter::CAction*	clone() const
		{
			IPlatformScreen::CButtonInfo* info =
				IPrimaryScreen::CButtonInfo::alloc(*getInfo());
			return new CMouseButtonDownUpAction(info);
		}

	protected:
		// CMouseButtonAction overrides
		virtual const char*		formatName() const { return "mousebutton"; }
	};

	class CConditionDialog {
	public:
		static bool		doModal(HWND parent, CInputFilter::CCondition*&);

	private:
		static void		doInit(HWND hwnd);
		static void		fillHotkey(HWND hwnd);

		static void		onButton(HWND hwnd, ButtonID button);
		static void		onKey(HWND hwnd, WPARAM wParam, LPARAM lParam);
		static KeyID	getChar(WPARAM wParam, LPARAM lParam);
		static KeyModifierMask
						getModifiers();

		static bool		isGoodCondition();

		static BOOL CALLBACK	dlgProc(HWND, UINT, WPARAM, LPARAM);
		static LRESULT CALLBACK	editProc(HWND hwnd, UINT, WPARAM, LPARAM);

	private:
		static CInputFilter::CCondition*
								s_condition;
		static CInputFilter::CCondition*
								s_lastGoodCondition;
		static WNDPROC			s_editWndProc;
	};

	class CActionDialog {
	public:
		static bool		doModal(HWND parent, CConfig* config,
							CInputFilter::CAction*&, bool& onActivate);

	private:
		static void		doInit(HWND hwnd);
		static void		fillHotkey(HWND hwnd);
		static void		updateControls(HWND hwnd);

		static void		onButton(HWND hwnd, ButtonID button);
		static void		onKey(HWND hwnd, WPARAM wParam, LPARAM lParam);
		static void		onLockAction(HWND hwnd);
		static void		onSwitchToAction(HWND hwnd);
		static void		onSwitchInAction(HWND hwnd);
		static void		onKeyboardBroadcastAction(HWND hwnd);

		static KeyID	getChar(WPARAM wParam, LPARAM lParam);
		static KeyModifierMask
						getModifiers();

		static bool		isGoodAction();
		static void		convertAction(HWND hwnd);

		static bool		isDownUpAction();

		static BOOL CALLBACK	dlgProc(HWND, UINT, WPARAM, LPARAM);
		static LRESULT CALLBACK	editProc(HWND hwnd, UINT, WPARAM, LPARAM);

	private:
		static CConfig*			s_config;
		static bool				s_onActivate;
		static CInputFilter::CAction*
								s_action;
		static CInputFilter::CAction*
								s_lastGoodAction;
		static std::set<CString>	s_screens;
		static WNDPROC			s_editWndProc;
	};

// public to allow CActionDialog to use it
public:
	class CScreensDialog {
	public:
		static void		doModal(HWND parent, CConfig* config,
							CInputFilter::CKeystrokeAction*);

		// public due to compiler brokenness
		typedef std::set<CString> CScreens;

	private:

		static void		doInit(HWND hwnd);
		static void		doFini(HWND hwnd);
		static void		fillScreens(HWND hwnd);
		static void		updateControls(HWND hwnd);

		static void		add(HWND hwnd);
		static void		remove(HWND hwnd);

		static void		getSelected(HWND hwnd, UINT id,
							const CScreens& inScreens, CScreens& outScreens);

		static BOOL CALLBACK	dlgProc(HWND, UINT, WPARAM, LPARAM);

	private:
		static CConfig*							s_config;
		static CInputFilter::CKeystrokeAction*	s_action;
		static CScreens							s_nonTargets;
		static CScreens							s_targets;
		static CString							s_allScreens;
	};

private:
	static CHotkeyOptions*	s_singleton;

	HWND				m_parent;
	CConfig*			m_config;
	CInputFilter*		m_inputFilter;
	CInputFilter::CRule	m_activeRule;
	UInt32				m_activeRuleIndex;
};

#endif

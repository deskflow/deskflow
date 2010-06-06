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

#ifndef CSCREENSLINKS_H
#define CSCREENSLINKS_H

#include "CConfig.h"
#include "ProtocolTypes.h"
#include "CString.h"

#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>

//! Screens and links dialog for Microsoft Windows launcher
class CScreensLinks {
public:
	CScreensLinks(HWND parent, CConfig*);
	~CScreensLinks();

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
	typedef std::pair<CConfig::CCellEdge, CConfig::CCellEdge> CConfigLink;
	struct CEdgeLink {
	public:
		CEdgeLink();
		CEdgeLink(const CString& name, const CConfigLink&);

		bool			connect(CConfig*);
		bool			disconnect(CConfig*);
		void			rename(const CString& oldName, const CString& newName);

		bool			overlaps(const CConfig* config) const;
		bool			operator==(const CEdgeLink&) const;

	public:
		CString				m_srcName;
		EDirection			m_srcSide;
		CConfig::CInterval	m_srcInterval;
		CString				m_dstName;
		CConfig::CInterval	m_dstInterval;
	};
	typedef std::vector<CEdgeLink> CEdgeLinkList;

	void				init(HWND hwnd);
	bool				save(HWND hwnd);

	CString				getSelectedScreen(HWND hwnd) const;
	void				addScreen(HWND hwnd);
	void				editScreen(HWND hwnd);
	void				removeScreen(HWND hwnd);
	void				addLink(HWND hwnd);
	void				editLink(HWND hwnd);
	void				removeLink(HWND hwnd);

	void				updateScreens(HWND hwnd, const CString& name);
	void				updateScreensControls(HWND hwnd);
	void				updateLinks(HWND hwnd);
	void				updateLinksControls(HWND hwnd);

	void				changeSrcSide(HWND hwnd);
	void				changeSrcScreen(HWND hwnd);
	void				changeDstScreen(HWND hwnd);
	void				changeIntervalStart(HWND hwnd, int id,
							CConfig::CInterval&);
	void				changeIntervalEnd(HWND hwnd, int id,
							CConfig::CInterval&);

	void				selectScreen(HWND hwnd, int id, const CString& name);
	void				updateLinkEditControls(HWND hwnd,
							const CEdgeLink& link);
	void				updateLinkIntervalControls(HWND hwnd,
							const CEdgeLink& link);
	void				updateLink(HWND hwnd);
	void				updateLinkValid(HWND hwnd, const CEdgeLink& link);

	void				updateLinkView(HWND hwnd);

	HWND				createErrorBox(HWND parent);
	void				resizeErrorBoxes();
	void				resizeErrorBox(HWND box, HWND assoc);

	CString				formatIntervalValue(float) const;
	CString				formatInterval(const CConfig::CInterval&) const;
	CString				formatLink(const CEdgeLink&) const;

	// message handling
	BOOL				doDlgProc(HWND, UINT, WPARAM, LPARAM);
	static BOOL CALLBACK dlgProc(HWND, UINT, WPARAM, LPARAM);

private:
	static CScreensLinks*	s_singleton;

	HWND				m_parent;
	CConfig*			m_mainConfig;
	CConfig				m_scratchConfig;
	CConfig*			m_config;

	CString				m_linkFormat;
	CString				m_intervalFormat;
	CString				m_newLinkLabel;
	CString				m_sideLabel[kNumDirections];
	CEdgeLinkList		m_edgeLinks;
	SInt32				m_selectedLink;
	CEdgeLink			m_editedLink;
	bool				m_editedLinkIsValid;
	HPEN				m_redPen;
	HWND				m_srcSideError;
	HWND				m_srcScreenError;
	HWND				m_dstScreenError;
};

#endif

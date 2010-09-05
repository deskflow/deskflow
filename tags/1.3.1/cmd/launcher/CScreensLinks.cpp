/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2002 Chris Schoeneman
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

#include "CConfig.h"
#include "ProtocolTypes.h"
#include "CStringUtil.h"
#include "CArch.h"
#include "CScreensLinks.h"
#include "CAddScreen.h"
#include "LaunchUtil.h"
#include "resource.h"

//
// CScreensLinks
//

CScreensLinks*		CScreensLinks::s_singleton = NULL;

CScreensLinks::CScreensLinks(HWND parent, CConfig* config) :
	m_parent(parent),
	m_mainConfig(config),
	m_config(&m_scratchConfig)
{
	assert(s_singleton == NULL);
	s_singleton = this;

	// get formatting strings
	m_linkFormat     = getString(IDS_LINK_FORMAT);
	m_intervalFormat = getString(IDS_LINK_INTERVAL_FORMAT);
	m_newLinkLabel   = getString(IDS_NEW_LINK);
	m_sideLabel[kLeft   - kFirstDirection] = getString(IDS_SIDE_LEFT);
	m_sideLabel[kRight  - kFirstDirection] = getString(IDS_SIDE_RIGHT);
	m_sideLabel[kTop    - kFirstDirection] = getString(IDS_SIDE_TOP);
	m_sideLabel[kBottom - kFirstDirection] = getString(IDS_SIDE_BOTTOM);

	// GDI objects
	m_redPen = CreatePen(PS_INSIDEFRAME, 1, RGB(255, 0, 0));
}

CScreensLinks::~CScreensLinks()
{
	DeleteObject(m_redPen);
	s_singleton = NULL;
}

void
CScreensLinks::doModal()
{
	// do dialog
	DialogBoxParam(s_instance, MAKEINTRESOURCE(IDD_SCREENS_LINKS),
								m_parent, dlgProc, (LPARAM)this);
}

void
CScreensLinks::init(HWND hwnd)
{
	// get initial config
	m_scratchConfig = *m_mainConfig;

	// fill side list box (in EDirection order)
	HWND child = getItem(hwnd, IDC_SCREENS_SRC_SIDE);
	SendMessage(child, CB_ADDSTRING, 0, (LPARAM)TEXT("---"));
	SendMessage(child, CB_ADDSTRING, 0,
							(LPARAM)getString(IDS_EDGE_LEFT).c_str());
	SendMessage(child, CB_ADDSTRING, 0,
							(LPARAM)getString(IDS_EDGE_RIGHT).c_str());
	SendMessage(child, CB_ADDSTRING, 0,
							(LPARAM)getString(IDS_EDGE_TOP).c_str());
	SendMessage(child, CB_ADDSTRING, 0,
							(LPARAM)getString(IDS_EDGE_BOTTOM).c_str());

	// create error boxes
	m_srcSideError   = createErrorBox(hwnd);
	m_srcScreenError = createErrorBox(hwnd);
	m_dstScreenError = createErrorBox(hwnd);
	resizeErrorBoxes();

	m_selectedLink = -1;
	m_editedLink   = CEdgeLink();
	m_edgeLinks.clear();
	updateScreens(hwnd, "");
	updateScreensControls(hwnd);
	updateLinks(hwnd);
	updateLinksControls(hwnd);
}

bool
CScreensLinks::save(HWND /*hwnd*/)
{
	*m_mainConfig = m_scratchConfig;
	return true;
}

BOOL
CScreensLinks::doDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
	case WM_INITDIALOG:
		init(hwnd);
		return TRUE;

	case WM_SIZE:
		resizeErrorBoxes();
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			SetFocus(getItem(hwnd, IDOK));
			if (save(hwnd)) {
				EndDialog(hwnd, 0);
			}
			return TRUE;

		case IDCANCEL:
			EndDialog(hwnd, 0);
			return TRUE;

		case IDC_SCREENS_SCREENS:
			switch (HIWORD(wParam)) {
			case LBN_DBLCLK:
				editScreen(hwnd);
				return TRUE;

			case LBN_SELCHANGE:
				updateScreensControls(hwnd);
				updateLinkView(hwnd);
				return TRUE;

			case LBN_SELCANCEL:
				updateScreensControls(hwnd);
				updateLinkView(hwnd);
				return TRUE;
			}
			break;

		case IDC_SCREENS_ADD_SCREEN:
			addScreen(hwnd);
			return TRUE;

		case IDC_SCREENS_REMOVE_SCREEN:
			removeScreen(hwnd);
			return TRUE;

		case IDC_SCREENS_EDIT_SCREEN:
			editScreen(hwnd);
			return TRUE;

		case IDC_SCREENS_LINKS:
			switch (HIWORD(wParam)) {
			case LBN_SELCHANGE:
				editLink(hwnd);
				return TRUE;

			case LBN_SELCANCEL:
				editLink(hwnd);
				return TRUE;
			}
			break;

		case IDC_SCREENS_ADD_LINK:
			addLink(hwnd);
			return TRUE;

		case IDC_SCREENS_REMOVE_LINK:
			removeLink(hwnd);
			return TRUE;

		case IDC_SCREENS_SRC_SIDE:
			switch (HIWORD(wParam)) {
			case CBN_SELCHANGE:
				changeSrcSide(hwnd);
				break;
			}
			break;

		case IDC_SCREENS_SRC_SCREEN:
			switch (HIWORD(wParam)) {
			case CBN_SELCHANGE:
				changeSrcScreen(hwnd);
				break;
			}
			break;

		case IDC_SCREENS_DST_SCREEN:
			switch (HIWORD(wParam)) {
			case CBN_SELCHANGE:
				changeDstScreen(hwnd);
				break;
			}
			break;

		case IDC_SCREENS_SRC_START:
			switch (HIWORD(wParam)) {
			case EN_KILLFOCUS:
				changeIntervalStart(hwnd, LOWORD(wParam),
							m_editedLink.m_srcInterval);
				break;
			}
			break;

		case IDC_SCREENS_SRC_END:
			switch (HIWORD(wParam)) {
			case EN_KILLFOCUS:
				changeIntervalEnd(hwnd, LOWORD(wParam),
							m_editedLink.m_srcInterval);
				break;
			}
			break;

		case IDC_SCREENS_DST_START:
			switch (HIWORD(wParam)) {
			case EN_KILLFOCUS:
				changeIntervalStart(hwnd, LOWORD(wParam),
							m_editedLink.m_dstInterval);
				break;
			}
			break;

		case IDC_SCREENS_DST_END:
			switch (HIWORD(wParam)) {
			case EN_KILLFOCUS:
				changeIntervalEnd(hwnd, LOWORD(wParam),
							m_editedLink.m_dstInterval);
				break;
			}
			break;
		}

		break;

	case WM_CTLCOLORSTATIC:
		switch (GetDlgCtrlID((HWND)lParam)) {
		case IDC_SCREENS_OVERLAP_ERROR:
			SetBkColor((HDC)wParam, GetSysColor(COLOR_3DFACE));
			SetTextColor((HDC)wParam, RGB(255, 0, 0));
			return (BOOL)GetSysColorBrush(COLOR_3DFACE);
		}
		break;

	// error outlines
	case WM_DRAWITEM: {
		DRAWITEMSTRUCT* di = (DRAWITEMSTRUCT*)lParam;
		if (di->CtlType == ODT_STATIC) {
			HGDIOBJ oldPen   = SelectObject(di->hDC, m_redPen);
			HGDIOBJ oldBrush = SelectObject(di->hDC,
										GetStockObject(NULL_BRUSH));
			Rectangle(di->hDC, di->rcItem.left, di->rcItem.top,
							di->rcItem.right, di->rcItem.bottom);
			SelectObject(di->hDC, oldPen);
			SelectObject(di->hDC, oldBrush);
			return TRUE;
		}
		break;
	}

	default:
		break;
	}

	return FALSE;
}

BOOL CALLBACK
CScreensLinks::dlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return s_singleton->doDlgProc(hwnd, message, wParam, lParam);
}

CString
CScreensLinks::getSelectedScreen(HWND hwnd) const
{
	HWND child    = getItem(hwnd, IDC_SCREENS_SCREENS);
	LRESULT index = SendMessage(child, LB_GETCURSEL, 0, 0);
	if (index == LB_ERR) {
		return CString();
	}

	LRESULT size = SendMessage(child, LB_GETTEXTLEN, index, 0);
	char* buffer = new char[size + 1];
	SendMessage(child, LB_GETTEXT, index, (LPARAM)buffer);
	buffer[size] = '\0';
	CString result(buffer);
	delete[] buffer;
	return result;
}

void
CScreensLinks::addScreen(HWND hwnd)
{
	CAddScreen dialog(hwnd, m_config, "");
	if (dialog.doModal()) {
		updateScreens(hwnd, dialog.getName());
		updateScreensControls(hwnd);
		updateLinks(hwnd);
		updateLinksControls(hwnd);
	}
}

void
CScreensLinks::editScreen(HWND hwnd)
{
	CString oldName = getSelectedScreen(hwnd);
	CAddScreen dialog(hwnd, m_config, oldName);
	if (dialog.doModal()) {
		CString newName = dialog.getName();

		// rename screens in the edge list
		if (newName != oldName) {
			for (size_t i = 0; i < m_edgeLinks.size(); ++i) {
				m_edgeLinks[i].rename(oldName, newName);
			}
			m_editedLink.rename(oldName, newName);
		}

		updateScreens(hwnd, newName);
		updateScreensControls(hwnd);
		updateLinks(hwnd);
		updateLinksControls(hwnd);
	}
}

void
CScreensLinks::removeScreen(HWND hwnd)
{
	// remove screen from config (this also removes aliases)
	m_config->removeScreen(getSelectedScreen(hwnd));

	// update dialog
	updateScreens(hwnd, "");
	updateScreensControls(hwnd);
	updateLinks(hwnd);
	updateLinksControls(hwnd);
}

void
CScreensLinks::addLink(HWND hwnd)
{
	if (m_editedLink.connect(m_config)) {
		m_editedLink = CEdgeLink();
		updateLinks(hwnd);
		updateLinksControls(hwnd);
	}
}

void
CScreensLinks::editLink(HWND hwnd)
{
	// get selection
	HWND child = getItem(hwnd, IDC_SCREENS_LINKS);
	DWORD i = SendMessage(child, LB_GETCURSEL, 0, 0);
	if (i != LB_ERR && i != (DWORD)m_edgeLinks.size()) {
		// existing link
		m_selectedLink = (SInt32)SendMessage(child, LB_GETITEMDATA, i, 0);
		m_editedLink   = m_edgeLinks[m_selectedLink];
	}
	else {
		// new link
		m_selectedLink = -1;
		m_editedLink   = CEdgeLink();
	}
	updateLinksControls(hwnd);
}

void
CScreensLinks::removeLink(HWND hwnd)
{
	if (m_editedLink.disconnect(m_config)) {
		updateLinks(hwnd);
		updateLinksControls(hwnd);
	}
}

void
CScreensLinks::updateScreens(HWND hwnd, const CString& selectName)
{
	HWND child;

	// set screen list
	child = getItem(hwnd, IDC_SCREENS_SCREENS);
	SendMessage(child, LB_RESETCONTENT, 0, 0);
	for (CConfig::const_iterator index = m_config->begin();
							index != m_config->end(); ) {
		const CString& name = *index;
		++index;
		if (index != m_config->end()) {
			SendMessage(child, LB_INSERTSTRING,
							(WPARAM)-1, (LPARAM)name.c_str());
		}
		else {
			SendMessage(child, LB_ADDSTRING, 0, (LPARAM)name.c_str());
		}
	}

	// find the named screen
	if (!selectName.empty()) {
		DWORD i = SendMessage(child, LB_FINDSTRINGEXACT,
							(UINT)-1, (LPARAM)selectName.c_str());
		if (i != LB_ERR) {
			SendMessage(child, LB_SETSEL, TRUE, i);
		}
	}
}

void
CScreensLinks::updateScreensControls(HWND hwnd)
{
	HWND child    = getItem(hwnd, IDC_SCREENS_SCREENS);
	bool screenSelected = (SendMessage(child, LB_GETCURSEL, 0, 0) != LB_ERR);

	enableItem(hwnd, IDC_SCREENS_ADD_SCREEN, TRUE);
	enableItem(hwnd, IDC_SCREENS_EDIT_SCREEN, screenSelected);
	enableItem(hwnd, IDC_SCREENS_REMOVE_SCREEN, screenSelected);
}

void
CScreensLinks::updateLinks(HWND hwnd)
{
	HWND links      = getItem(hwnd, IDC_SCREENS_LINKS);
	HWND srcScreens = getItem(hwnd, IDC_SCREENS_SRC_SCREEN);
	HWND dstScreens = getItem(hwnd, IDC_SCREENS_DST_SCREEN);

	// get old selection
	CEdgeLink oldLink;
	if (m_selectedLink != -1) {
		oldLink = m_edgeLinks[m_selectedLink];
	}

	// clear links and screens
	SendMessage(links, LB_RESETCONTENT, 0, 0);
	SendMessage(srcScreens, CB_RESETCONTENT, 0, 0);
	SendMessage(dstScreens, CB_RESETCONTENT, 0, 0);
	m_edgeLinks.clear();

	// add "no screen" items
	SendMessage(srcScreens, CB_INSERTSTRING, (WPARAM)-1, (LPARAM)TEXT("----"));
	SendMessage(dstScreens, CB_INSERTSTRING, (WPARAM)-1, (LPARAM)TEXT("----"));

	// add links and screens
	for (CConfig::const_iterator i = m_config->begin();
							i != m_config->end(); ++i) {
		const CString& name = *i;

		// add screen
		SendMessage(srcScreens, CB_INSERTSTRING, (WPARAM)-1,
							(LPARAM)name.c_str());
		SendMessage(dstScreens, CB_INSERTSTRING, (WPARAM)-1,
							(LPARAM)name.c_str());

		// add links for screen
		for (CConfig::link_const_iterator j = m_config->beginNeighbor(name),
										n = m_config->endNeighbor(name);
							j != n; ++j) {
			DWORD k = m_edgeLinks.size();
			m_edgeLinks.push_back(CEdgeLink(name, *j));
			SendMessage(links, LB_INSERTSTRING, (WPARAM)-1,
							(LPARAM)formatLink(m_edgeLinks.back()).c_str());
			SendMessage(links, LB_SETITEMDATA, (WPARAM)k, (LPARAM)k);
		}
	}

	// add "new link" item to sort
	SendMessage(links, LB_ADDSTRING, 0, (LPARAM)m_newLinkLabel.c_str());

	// remove the "new link" item then insert it on the end
	DWORD i = SendMessage(links, LB_FINDSTRINGEXACT,
							(UINT)-1, (LPARAM)m_newLinkLabel.c_str());
	if (i != LB_ERR) {
		SendMessage(links, LB_DELETESTRING, i, 0);
	}
	SendMessage(links, LB_INSERTSTRING, (WPARAM)-1,
							(LPARAM)getString(IDS_NEW_LINK).c_str());
	SendMessage(links, LB_SETITEMDATA, (WPARAM)m_edgeLinks.size(),
							(LPARAM)-1);

	// select the same link as before
	SendMessage(links, LB_SETCURSEL, (WPARAM)m_edgeLinks.size(), 0);
	if (m_selectedLink != -1) {
		m_selectedLink = -1;
		for (size_t j = 0; j < m_edgeLinks.size(); ++j) {
			if (m_edgeLinks[j] == oldLink) {
				// found matching link
				m_selectedLink = j;
				for (size_t k = 0; k < m_edgeLinks.size(); ++k) {
					if (SendMessage(links, LB_GETITEMDATA, k, 0) == (int)j) {
						SendMessage(links, LB_SETCURSEL, k, 0);
						break;
					}
				}
				break;
			}
		}

		// if we can't find the link anymore then reset edited link
		if (m_selectedLink == -1) {
			m_editedLink = CEdgeLink();
		}
	}
}

void
CScreensLinks::updateLinksControls(HWND hwnd)
{
	// get selection.  select "new link" if nothing is selected.
	HWND child = getItem(hwnd, IDC_SCREENS_LINKS);
	if (m_selectedLink == -1) {
		SendMessage(child, LB_SETCURSEL, m_edgeLinks.size(), 0);
	}

	// enable/disable remove button
	enableItem(hwnd, IDC_SCREENS_REMOVE_LINK, m_selectedLink != -1);

	// fill link entry controls from m_editedLink
	updateLinkEditControls(hwnd, m_editedLink);
	updateLinkValid(hwnd, m_editedLink);
	updateLinkView(hwnd);
}

void
CScreensLinks::changeSrcSide(HWND hwnd)
{
	HWND child = getItem(hwnd, IDC_SCREENS_SRC_SIDE);
	m_editedLink.m_srcSide = (EDirection)SendMessage(child, CB_GETCURSEL, 0, 0);
	updateLink(hwnd);
}

void
CScreensLinks::changeSrcScreen(HWND hwnd)
{
	HWND child = getItem(hwnd, IDC_SCREENS_SRC_SCREEN);
	m_editedLink.m_srcName = getWindowText(child);
	updateLink(hwnd);
}

void
CScreensLinks::changeDstScreen(HWND hwnd)
{
	HWND child = getItem(hwnd, IDC_SCREENS_DST_SCREEN);
	m_editedLink.m_dstName = getWindowText(child);
	updateLink(hwnd);
}

void
CScreensLinks::changeIntervalStart(HWND hwnd, int id, CConfig::CInterval& i)
{
	int x = (int)GetDlgItemInt(hwnd, id, NULL, FALSE);
	if (x < 0) {
		x = 0;
	}
	else if (x > 99) {
		x = 99;
	}

	i.first = 0.01f * (float)x;
	if (i.first >= i.second) {
		i.second = 0.01f * (float)(x + 1);
	}

	updateLinkIntervalControls(hwnd, m_editedLink);
	updateLink(hwnd);
}

void
CScreensLinks::changeIntervalEnd(HWND hwnd, int id, CConfig::CInterval& i)
{
	int x = (int)GetDlgItemInt(hwnd, id, NULL, FALSE);
	if (x < 1) {
		x = 1;
	}
	else if (x > 100) {
		x = 100;
	}

	i.second = 0.01f * (float)x;
	if (i.first >= i.second) {
		i.first = 0.01f * (float)(x - 1);
	}

	updateLinkIntervalControls(hwnd, m_editedLink);
	updateLink(hwnd);
}

void
CScreensLinks::selectScreen(HWND hwnd, int id, const CString& name)
{
	HWND child = getItem(hwnd, id);
	DWORD i = SendMessage(child, CB_FINDSTRINGEXACT, (WPARAM)-1,
							(LPARAM)name.c_str());
	if (i == CB_ERR) {
		// no match, select no screen
		SendMessage(child, CB_SETCURSEL, 0, 0);
	}
	else {
		SendMessage(child, CB_SETCURSEL, i, 0);
	}
}

void
CScreensLinks::updateLinkEditControls(HWND hwnd, const CEdgeLink& link)
{
	// fill link entry controls from link
	HWND child = getItem(hwnd, IDC_SCREENS_SRC_SIDE);
	SendMessage(child, CB_SETCURSEL, link.m_srcSide, 0);
	selectScreen(hwnd, IDC_SCREENS_SRC_SCREEN, link.m_srcName);
	selectScreen(hwnd, IDC_SCREENS_DST_SCREEN, link.m_dstName);
	updateLinkIntervalControls(hwnd, link);
}

void
CScreensLinks::updateLinkIntervalControls(HWND hwnd, const CEdgeLink& link)
{
	HWND child;

	// src interval
	child = getItem(hwnd, IDC_SCREENS_SRC_START);
	setWindowText(child, formatIntervalValue(link.m_srcInterval.first));
	child = getItem(hwnd, IDC_SCREENS_SRC_END);
	setWindowText(child, formatIntervalValue(link.m_srcInterval.second));

	// dst interval
	child = getItem(hwnd, IDC_SCREENS_DST_START);
	setWindowText(child, formatIntervalValue(link.m_dstInterval.first));
	child = getItem(hwnd, IDC_SCREENS_DST_END);
	setWindowText(child, formatIntervalValue(link.m_dstInterval.second));
}

void
CScreensLinks::updateLink(HWND hwnd)
{
	updateLinkValid(hwnd, m_editedLink);

	// update link in config
	if (m_selectedLink != -1 && m_editedLinkIsValid) {
		// editing an existing link and entry is valid
		if (m_edgeLinks[m_selectedLink].disconnect(m_config)) {
			// successfully removed old link
			if (!m_editedLink.connect(m_config)) {
				// couldn't set new link so restore old link
				m_edgeLinks[m_selectedLink].connect(m_config);
			}
			else {
				m_edgeLinks[m_selectedLink] = m_editedLink;
				updateLinks(hwnd);
				updateLinkEditControls(hwnd, m_editedLink);
			}
		}
	}

	updateLinkView(hwnd);
}

void
CScreensLinks::updateLinkValid(HWND hwnd, const CEdgeLink& link)
{
	m_editedLinkIsValid = true;

	// check source side and screen
	if (link.m_srcSide == kNoDirection) {
		m_editedLinkIsValid = false;
		ShowWindow(m_srcSideError, SW_SHOWNA);
	}
	else {
		ShowWindow(m_srcSideError, SW_HIDE);
	}
	if (!m_config->isCanonicalName(link.m_srcName)) {
		m_editedLinkIsValid = false;
		ShowWindow(m_srcScreenError, SW_SHOWNA);
	}
	else {
		ShowWindow(m_srcScreenError, SW_HIDE);
	}

	// check for overlap.  if editing a link we must remove it, then
	// check for overlap and restore the old link.
	bool overlap = false;
	if (m_editedLinkIsValid) {
		if (m_selectedLink == -1) {
			if (link.overlaps(m_config)) {
				m_editedLinkIsValid = false;
				overlap = true;
			}
		}
		else {
			if (m_edgeLinks[m_selectedLink].disconnect(m_config)) {
				overlap = link.overlaps(m_config);
				m_edgeLinks[m_selectedLink].connect(m_config);
				if (overlap) {
					m_editedLinkIsValid = false;
				}
			}
		}
	}
	ShowWindow(getItem(hwnd, IDC_SCREENS_OVERLAP_ERROR),
							overlap ? SW_SHOWNA : SW_HIDE);

	// check dst screen
	if (!m_config->isCanonicalName(link.m_dstName)) {
		m_editedLinkIsValid = false;
		ShowWindow(m_dstScreenError, SW_SHOWNA);
	}
	else {
		ShowWindow(m_dstScreenError, SW_HIDE);
	}

	// update add link button
	enableItem(hwnd, IDC_SCREENS_ADD_LINK,
							m_selectedLink == -1 && m_editedLinkIsValid);
}

void
CScreensLinks::updateLinkView(HWND /*hwnd*/)
{
	// XXX -- draw visual of selected screen, highlighting selected link
}

HWND
CScreensLinks::createErrorBox(HWND parent)
{
	return CreateWindow(TEXT("STATIC"), TEXT(""),
							WS_CHILD | SS_OWNERDRAW,
							0, 0, 1, 1,
							parent, (HMENU)-1,
							s_instance, NULL);
}

void
CScreensLinks::resizeErrorBoxes()
{
	HWND hwnd = GetParent(m_srcSideError);
	resizeErrorBox(m_srcSideError,   getItem(hwnd, IDC_SCREENS_SRC_SIDE));
	resizeErrorBox(m_srcScreenError, getItem(hwnd, IDC_SCREENS_SRC_SCREEN));
	resizeErrorBox(m_dstScreenError, getItem(hwnd, IDC_SCREENS_DST_SCREEN));
}

void
CScreensLinks::resizeErrorBox(HWND box, HWND assoc)
{
	RECT rect;
	GetWindowRect(assoc, &rect);
	MapWindowPoints(NULL, GetParent(box), (POINT*)&rect, 2);
	SetWindowPos(box, HWND_TOP, rect.left - 1, rect.top - 1,
							rect.right - rect.left + 2,
							rect.bottom - rect.top + 2, SWP_NOACTIVATE);
}

CString
CScreensLinks::formatIntervalValue(float x) const
{
	return CStringUtil::print("%d", (int)(x * 100.0f + 0.5f));
}

CString
CScreensLinks::formatInterval(const CConfig::CInterval& i) const
{
	if (i.first == 0.0f && i.second == 1.0f) {
		return "";
	}
	else {
		CString start = formatIntervalValue(i.first);
		CString end   = formatIntervalValue(i.second);
		return CStringUtil::format(m_intervalFormat.c_str(),
							start.c_str(), end.c_str());
	}
}

CString
CScreensLinks::formatLink(const CEdgeLink& link) const
{
	CString srcInterval = formatInterval(link.m_srcInterval);
	CString dstInterval = formatInterval(link.m_dstInterval);
	return CStringUtil::format(m_linkFormat.c_str(),
						link.m_srcName.c_str(), srcInterval.c_str(),
						m_sideLabel[link.m_srcSide - kFirstDirection].c_str(),
						link.m_dstName.c_str(), dstInterval.c_str());
}

//
// CScreensLinks::CEdgeLink
//

CScreensLinks::CEdgeLink::CEdgeLink() :
	m_srcName(),
	m_srcSide(kNoDirection),
	m_srcInterval(0.0f, 1.0f),
	m_dstName(),
	m_dstInterval(0.0f, 1.0f)
{
	// do nothing
}

CScreensLinks::CEdgeLink::CEdgeLink(const CString& name,
							const CConfigLink& link) :
	m_srcName(name),
	m_srcSide(link.first.getSide()),
	m_srcInterval(link.first.getInterval()),
	m_dstName(link.second.getName()),
	m_dstInterval(link.second.getInterval())
{
	// do nothing
}

bool
CScreensLinks::CEdgeLink::connect(CConfig* config)
{
	return config->connect(m_srcName, m_srcSide,
							m_srcInterval.first, m_srcInterval.second,
							m_dstName,
							m_dstInterval.first, m_dstInterval.second);
}

bool
CScreensLinks::CEdgeLink::disconnect(CConfig* config)
{
	return config->disconnect(m_srcName, m_srcSide, 0.5f *
							(m_srcInterval.first + m_srcInterval.second));
}

void
CScreensLinks::CEdgeLink::rename(const CString& oldName, const CString& newName)
{
	if (m_srcName == oldName) {
		m_srcName = newName;
	}
	if (m_dstName == oldName) {
		m_dstName = newName;
	}
}

bool
CScreensLinks::CEdgeLink::overlaps(const CConfig* config) const
{
	return config->hasNeighbor(m_srcName, m_srcSide,
							m_srcInterval.first, m_srcInterval.second);
}

bool
CScreensLinks::CEdgeLink::operator==(const CEdgeLink& x) const
{
	return (m_srcName == x.m_srcName &&
			m_srcSide == x.m_srcSide &&
			m_srcInterval == x.m_srcInterval &&
			m_dstName == x.m_dstName &&
			m_dstInterval == x.m_dstInterval);
}

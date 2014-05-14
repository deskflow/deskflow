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

#include "common/stdvector.h"
#include "base/String.h"
#include "base/EventTypes.h"

class CDragInformation;
typedef std::vector<CDragInformation> CDragFileList;

class CDragInformation {
public:
	CDragInformation();
	~CDragInformation() { }
	
	CString&			getFilename() { return m_filename; }
	void				setFilename(CString& name) { m_filename = name; }
	size_t				getFilesize() { return m_filesize; }
	void				setFilesize(size_t size) { m_filesize = size; }
	
	static void			parseDragInfo(CDragFileList& dragFileList, UInt32 fileNum, CString data);
	static CString		getDragFileExtension(CString filename);
	// helper function to setup drag info
	// example: filename1,filesize1,filename2,filesize2,
	// return file count
	static int			setupDragInfo(CDragFileList& fileList, CString& output);

	static bool			isFileValid(CString filename);

private:
	static size_t		stringToNum(CString& str);
	static CString		getFileSize(CString& filename);

private:
	CString				m_filename;
	size_t				m_filesize;
};

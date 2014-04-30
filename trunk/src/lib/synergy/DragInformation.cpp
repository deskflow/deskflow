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

#include "synergy/DragInformation.h"
#include "base/Log.h"

using namespace std;

void
CDragInformation::parseDragInfo(CDragFileList& dragFileList, UInt32 fileNum, CString data)
{
	size_t startPos = 0;
	size_t findResult1 = 0;
	size_t findResult2 = 0;
	dragFileList.clear();
	CString slash("\\");
	if (data.find("/", startPos) != string::npos) {
		slash = "/";
	}

	while (fileNum) {
		findResult1 = data.find('\0', startPos);
		findResult2 = data.find_last_of(slash, findResult1);

		if (findResult1 == startPos) {
			//TODO: file number does not match, something goes wrong
			break;
		}
		if (findResult1 - findResult2 > 1) {
			dragFileList.push_back(data.substr(findResult2 + 1, findResult1 - findResult2));
		}
		startPos = findResult1 + 1;
		--fileNum;
	}

	LOG((CLOG_DEBUG "drag info received, total drag file number: %i", dragFileList.size()));

	for (size_t i = 0; i < dragFileList.size(); ++i) {
		LOG((CLOG_DEBUG2 "dragging file %i name: %s", i + 1, dragFileList.at(i).c_str()));
	}
}

CString
CDragInformation::getDragFileExtension(CString fileName)
{
	size_t findResult = string::npos;
	findResult = fileName.find_last_of(".", fileName.size());
	if (findResult != string::npos) {
		return fileName.substr(findResult + 1, fileName.size() - findResult - 1);
	}
	else {
		return "";
	}
}

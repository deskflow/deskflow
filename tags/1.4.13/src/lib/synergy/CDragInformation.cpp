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

#include "CDragInformation.h"
#include "CLog.h"

using namespace std;

void
CDragInformation::parseDragInfo(CDragFileList& dragFileList, UInt32 fileNum, CString data)
{
	size_t startPos = 0;
	size_t findResult1 = 0;
	size_t findResult2 = 0;
	dragFileList.clear();
	CString slash("\\");
	if (data.find("/", startPos) != -1) {
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
}

CString
CDragInformation::getDragFileExtension(CString fileName)
{
	size_t findResult = -1;
	findResult = fileName.find_last_of(".", fileName.size());
	if (findResult != -1) {
		return fileName.substr(findResult + 1, fileName.size() - findResult - 1);
	}
	else {
		return "";
	}
}

/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2014 Synergy Si Ltd.
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
 * 
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the OpenSSL
 * library.
 * You must obey the GNU General Public License in all respects for all of
 * the code used other than OpenSSL. If you modify file(s) with this
 * exception, you may extend this exception to your version of the file(s),
 * but you are not obligated to do so. If you do not wish to do so, delete
 * this exception statement from your version. If you delete this exception
 * statement from all source files in the program, then also delete it here.
 */

#include "synergy/DropHelper.h"

#include "base/Log.h"

#include <fstream>

void
DropHelper::writeToDir(const String& destination, DragFileList& fileList, String& data)
{
	LOG((CLOG_DEBUG "dropping file, files=%i target=%s", fileList.size(), destination.c_str()));

	if (!destination.empty() && fileList.size() > 0) {
		std::fstream file;
		String dropTarget = destination;
#ifdef SYSAPI_WIN32
		dropTarget.append("\\");
#else
		dropTarget.append("/");
#endif
		dropTarget.append(fileList.at(0).getFilename());
		file.open(dropTarget.c_str(), std::ios::out | std::ios::binary);
		if (!file.is_open()) {
			LOG((CLOG_ERR "drop file failed: can not open %s", dropTarget.c_str()));
		}
		
		file.write(data.c_str(), data.size());
		file.close();

		LOG((CLOG_DEBUG "%s is saved to %s", fileList.at(0).getFilename().c_str(), destination.c_str()));

		fileList.clear();
	}
	else {
		LOG((CLOG_ERR "drop file failed: drop target is empty"));
	}
}

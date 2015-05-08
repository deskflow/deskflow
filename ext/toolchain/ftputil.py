# synergy -- mouse and keyboard sharing utility
# Copyright (C) 2012 Synergy Si Ltd.
# Copyright (C) 2010 Nick Bolton
# 
# This package is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# found in the file LICENSE that should have accompanied this file.
# 
# This package is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

from ftplib import FTP

class FtpUploader:
	def __init__(self, host, user, password, dir):
		self.host = host
		self.user = user
		self.password = password
		self.dir = dir

	def run(self, src, dest, replace=False):
		
		ftp = FTP(self.host, self.user, self.password)
		ftp.cwd(self.dir)
		
		f = open(src, 'rb')
		ftp.storbinary('STOR ' + dest, f)
		f.close()
		
		ftp.close()

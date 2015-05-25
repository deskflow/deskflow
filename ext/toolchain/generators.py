# synergy -- mouse and keyboard sharing utility
# Copyright (C) 2012 Synergy Si Ltd.
# Copyright (C) 2009 Nick Bolton
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

class Generator(object):
	def __init__(self, cmakeName, buildDir='build', sourceDir='..', binDir='bin'):
		self.cmakeName = cmakeName
        	self.buildDir = buildDir
		self.sourceDir = sourceDir
		self.binDir = binDir

	def getBuildDir(self, target):
		return self.buildDir

	def getBinDir(self, target=''):
		return self.binDir

	def getSourceDir(self):
		return self.sourceDir

class MakefilesGenerator(Generator):
	def __init__(self):
		super(MakefilesGenerator, self).__init__('Unix Makefiles')

	def getBuildDir(self, target):
		return super(MakefilesGenerator, self).getBuildDir(target) + '/' + target

	def getBinDir(self, target=''):
		workingDir = super(MakefilesGenerator, self).getBinDir(target)
		
		# only put debug files in separate bin dir
		if target == 'debug':
			workingDir += '/debug'
		
		return workingDir

	def getSourceDir(self):
		return super(MakefilesGenerator, self).getSourceDir() + '/..'
		
class XcodeGenerator(Generator):
	def __init__(self):
		super(XcodeGenerator, self).__init__('Xcode')
		
	def getBinDir(self, target=''):
		if target == "":
			return super(XcodeGenerator, self).getBinDir(target)
		
		xcodeTarget = target[0].upper() + target[1:]
		return super(XcodeGenerator, self).getBinDir(target) + '/' + xcodeTarget

class EclipseGenerator(Generator):
	def __init__(self):
		super(EclipseGenerator, self).__init__('Eclipse CDT4 - Unix Makefiles', '', '')

	def getBuildDir(self, target):
		# eclipse only works with in-source build.
		return ''

	def getBinDir(self, target=''):
		# eclipse only works with in-source build.
		return 'bin'

	def getSourceDir(self):
		return ''


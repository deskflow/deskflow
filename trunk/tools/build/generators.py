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
		super(MakefilesGenerator, self).__init__('Unix Makefiles', 'build', '..', 'bin')

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


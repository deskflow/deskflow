class Generator(object):
	def __init__(self, cmakeName, buildDir='build', sourceDir='..', binDir='bin'):
		self.cmakeName = cmakeName
        	self.buildDir = buildDir
		self.sourceDir = sourceDir
		self.binDir = binDir

	def getBuildDir(self, target):
		workingDir = self.buildDir
		if target != '':
			workingDir += '/' + target
		return workingDir

	def getBinDir(self, target=''):
		workingDir = self.binDir
		if target != '':
			workingDir += '/' + target
		return workingDir

	def getSourceDir(self):
		sourceDir = self.sourceDir
		if not self.cmakeName.startswith('Visual Studio'):
			sourceDir += '/..'
		return sourceDir

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


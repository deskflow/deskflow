# synergy -- mouse and keyboard sharing utility
# Copyright (C) 2012 Bolton Software Ltd.
# Copyright (C) 2009 Nick Bolton
# 
# This package is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# found in the file COPYING that should have accompanied this file.
# 
# This package is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# TODO: split this file up, it's too long!

import sys, os, ConfigParser, shutil, re, ftputil
from generators import Generator, EclipseGenerator, MakefilesGenerator

if sys.version_info >= (2, 4):
	import subprocess

class InternalCommands:
	
	project = 'synergy'
	setup_version = 5 # increment to force setup/config
	website_url = 'http://synergy-foss.org/'

	this_cmd = 'hm'
	cmake_cmd = 'cmake'
	qmake_cmd = 'qmake'
	make_cmd = 'make'
	xcodebuild_cmd = 'xcodebuild'
	w32_make_cmd = 'mingw32-make'
	w32_qt_version = '4.6.2'
	defaultTarget = 'release'

	cmake_dir = 'res'
	gui_dir = 'src/gui'
	doc_dir = 'doc'

	sln_filename = '%s.sln' % project
	xcodeproj_filename = '%s.xcodeproj' % project
	configDir = 'build'
	configFilename = '%s/%s.cfg' % (configDir, this_cmd)
	qtpro_filename = 'gui.pro'
	doxygen_filename = 'doxygen.cfg'

	cmake_url = 'http://www.cmake.org/cmake/resources/software.html'

	# try_chdir(...) and restore_chdir() will use this
	prevdir = ''
	
	# by default, no index specified as arg
	generator_id = None
	
	# by default, prompt user for input
	no_prompts = False
	
	# by default, don't compile the gui
	enable_make_gui = False
  
	# by default, do not compile with game device support.
	gameDevice = False
	
	# by default, do not compile with vnc support.
	vncSupport = False
	
	# by default, let cmake decide
	macSdk = None

	win32_generators = {
		1 : Generator('Visual Studio 10'),
		2 : Generator('Visual Studio 10 Win64'),
		3 : Generator('Visual Studio 9 2008'),
		4 : Generator('Visual Studio 9 2008 Win64'),
		5 : Generator('Visual Studio 8 2005'),
		6 : Generator('Visual Studio 8 2005 Win64')
	}

	unix_generators = {
		1 : MakefilesGenerator(),
		2 : EclipseGenerator(),
	}

	darwin_generators = {
		1 : MakefilesGenerator(),
		2 : Generator('Xcode'),
		3 : EclipseGenerator(),
	}

	def getBuildDir(self, target=''):
		return self.getGenerator().getBuildDir(target)

	def getBinDir(self, target=''):
		return self.getGenerator().getBinDir(target)

	def sln_filepath(self):
		return '%s\%s' % (self.getBuildDir(), self.sln_filename)

	def xcodeproj_filepath(self, target=''):
		return '%s/%s' % (self.getBuildDir(target), self.xcodeproj_filename)
		
	def usage(self):
		app = sys.argv[0]
		print ('Usage: %s <command> [-g <index>|-v|--no-prompts|<command-options>]\n'
			'\n'
			'Replace [command] with one of:\n'
			'  about       Show information about this script\n'
			'  setup       Runs the initial setup for this script\n'
			'  conf        Runs cmake (generates project files)\n'
			'  open        Attempts to open the generated project file\n'
			'  build       Builds using the platform build chain\n'
			'  clean       Cleans using the platform build chain\n'
			'  kill        Kills all synergy processes (run as admin)\n'
			'  update      Updates the source code from repository\n'
			'  revision    Display the current source code revision\n'
			'  package     Create a distribution package (e.g. tar.gz)\n'
			'  install     Installs the program\n'
			'  doxygen     Builds doxygen documentation\n'
			'  reformat    Reformat .cpp and .h files using AStyle\n'
			'  usage       Shows the help screen\n'
			'\n'
			'Example: %s build -g 3'
			) % (app, app)

	def configureAll(self, targets, extraArgs=''):

		# if no mode specified, use default
		if len(targets) == 0:
			targets += [self.defaultTarget,]

		for target in targets:
			self.configure(target)

	def configure(self, target='', extraArgs=''):
		
		cmake_args = ''

		# ensure latest setup and do not ask config for generator (only fall 
		# back to prompt if not specified as arg)
		self.ensure_setup_latest()
		
		# ensure that we have access to cmake
		_cmake_cmd = self.persist_cmake()

		# now that we know we've got the latest setup, we can ask the config
		# file for the generator (but again, we only fall back to this if not 
		# specified as arg).
		generator = self.getGenerator()
		
		if generator != self.findGeneratorFromConfig():
			print('Generator changed, running setup.')
			self.setup(target)

		if generator.cmakeName != '':
			cmake_args += ' -G "' + generator.cmakeName + '"'
		
		# default is release
		if target == '':
			print 'Defaulting target to: ' + self.defaultTarget
			target = self.defaultTarget

		# for makefiles always specify a build type (debug, release, etc)
		if generator.cmakeName.find('Unix Makefiles') != -1:
			cmake_args += ' -DCMAKE_BUILD_TYPE=' + target.capitalize()
		
		if self.gameDevice:
			cmake_args += " -DGAME_DEVICE_SUPPORT:BOOL=TRUE"
		else:
			cmake_args += " -DGAME_DEVICE_SUPPORT:BOOL=FALSE"
		
		if self.vncSupport:
			cmake_args += " -DVNC_SUPPORT:BOOL=TRUE"
		else:
			cmake_args += " -DVNC_SUPPORT:BOOL=FALSE"
		
		if self.macSdk:
			path = "/Developer/SDKs/MacOSX" + self.macSdk + ".sdk/"
			cmake_args += " -DCMAKE_OSX_SYSROOT=" + path
			cmake_args += " -DCMAKE_OSX_DEPLOYMENT_TARGET=" + self.macSdk
			os.environ["MACOSX_DEPLOYMENT_TARGET"] = self.macSdk

			# store the sdk version for the build command
			config = self.getConfig()
			config.set('cmake', 'mac_sdk', self.macSdk)
			self.write_config(config)
		
		# if not visual studio, use parent dir
		sourceDir = generator.getSourceDir()
		
		if extraArgs != '':
			cmake_args += ' ' + extraArgs

		cmake_cmd_string = _cmake_cmd + cmake_args + ' ' + sourceDir
		
		# Run from build dir so we have an out-of-source build.
		self.try_chdir(self.getBuildDir(target))

		print "CMake command: " + cmake_cmd_string
		err = os.system(cmake_cmd_string)

		self.restore_chdir()

		if generator.cmakeName.find('Eclipse') != -1:
			self.fixCmakeEclipseBug()

		if err != 0:
			raise Exception('CMake encountered error: ' + str(err))
		
		# allow user to skip qui compile
		if self.enable_make_gui:
			
			# make sure we have qmake
			self.persist_qmake()
			
			qmake_cmd_string = self.qmake_cmd + " " + self.qtpro_filename + " -r"

			if sys.platform == "darwin":
				# create makefiles on mac (not xcode).
				qmake_cmd_string += " -spec macx-g++"
				
				(major, minor) = self.getMacVersion()
				if major == 10 and minor <= 4:
					# 10.4: universal (intel and power pc)
					qmake_cmd_string += ' CONFIG+="ppc i386"'

			print "QMake command: " + qmake_cmd_string
			
			# run qmake from the gui dir
			self.try_chdir(self.gui_dir)
			err = os.system(qmake_cmd_string)
			self.restore_chdir()
			
			if err != 0:
				raise Exception('QMake encountered error: ' + str(err))
		
		self.setConfRun(target)

	# http://tinyurl.com/cs2rxxb
	def fixCmakeEclipseBug(self):
		print "Fixing CMake Eclipse bugs..."

		file = open('.project', 'r+')
		content = file.read()
		pattern = re.compile('\s+<linkedResources>.+</linkedResources>', re.S)
		content = pattern.sub('', content)
		file.seek(0)
		file.write(content)
		file.truncate()
		file.close()

	def persist_cmake(self):
		# even though we're running `cmake --version`, we're only doing this for the 0 return
		# code; we don't care about the version, since CMakeLists worrys about this for us.
		err = os.system('%s --version' % self.cmake_cmd)
		
		if err != 0:
			# if return code from cmake is not 0, then either something has
			# gone terribly wrong with --version, or it genuinely doesn't exist.
			print ('Could not find `%s` in system path.\n'
			       'Download the latest version from:\n  %s') % (
				self.cmake_cmd, self.cmake_url)
			raise Exception('Cannot continue without CMake.')
		else:	
			return self.cmake_cmd

	def persist_qt(self):
		self.persist_qmake()

	def persist_qmake(self):
		# cannot use subprocess on < python 2.4
		if sys.version_info < (2, 4):
			return
		
		try:
			p = subprocess.Popen(
				[self.qmake_cmd, '--version'], 
				stdout=subprocess.PIPE, 
				stderr=subprocess.PIPE)
		except:
			print >> sys.stderr, 'Error: Could not find qmake.'
			if sys.platform == 'win32': # windows devs usually need hints ;)
				print (
					'Suggestions:\n'
					'1. Ensure that qmake.exe exists in your system path.\n'
					'2. Try to download Qt (check our dev FAQ for links):\n'
					'  qt-sdk-win-opensource-2010.02.exe')
			raise Exception('Cannot continue without qmake.')
		
		stdout, stderr = p.communicate()
		if p.returncode != 0:
			raise Exception('Could not test for cmake: %s' % stderr)
		else:
			m = re.search('.*Using Qt version (\d+\.\d+\.\d+).*', stdout)
			if m:
				if sys.platform == 'win32':
					ver = m.group(1)
					if ver != self.w32_qt_version: # TODO: test properly
						print >> sys.stderr, (
							'Warning: Not using supported Qt version %s'
							' (your version is %s).'
							) % (self.w32_qt_version, ver)
				else:
					pass # any version should be ok for other platforms
			else:
				raise Exception('Could not find qmake version.')

	def ensureConfHasRun(self, target, skipConfig):
		if self.hasConfRun(target):
			print 'Skipping config for target: ' + target
			skipConfig = True

		if not skipConfig:
			self.configure(target)

	def build(self, targets=[], skipConfig=False):

		# if no mode specified, use default
		if len(targets) == 0:
			targets += [self.defaultTarget,]
	
		self.ensure_setup_latest()

		generator = self.getGeneratorFromConfig().cmakeName
		
		config = self.getConfig()
		if config.has_option("cmake", "mac_sdk"):
			macSdk = config.get("cmake", "mac_sdk")
			os.environ["MACOSX_DEPLOYMENT_TARGET"] = macSdk

		if generator.find('Unix Makefiles') != -1:
			for target in targets:
				self.runBuildCommand(self.make_cmd, target)
		else:
			for target in targets:
				if generator.startswith('Visual Studio'):
					self.run_vcbuild(generator, target)
				elif generator == 'Xcode':
					cmd = self.xcodebuild_cmd + ' -configuration ' + target.capitalize()
					self.runBuildCommand(cmd, target)
				else:
					raise Exception('Build command not supported with generator: ' + generator)

		# allow user to skip qui compile
		if self.enable_make_gui:
			self.make_gui(targets)
	
	def signmac(self, identity):
		self.try_chdir("bin")
		err = os.system(
			'codesign -fs "' + identity + '" Synergy.app')
		self.restore_chdir()
	
	def signwin(self, pfx, pwdFile, dist):
		generator = self.getGeneratorFromConfig().cmakeName
		if not generator.startswith('Visual Studio'):
			raise Exception('only windows is supported')
		
		f = open(pwdFile)
		lines = f.readlines()
		f.close()
		pwd = lines[0]
		
		if (dist):
			self.signFile(pfx, pwd, 'bin', self.dist_name('win'))
		else:
			self.signFile(pfx, pwd, 'bin/Release', 'synergy.exe')
			self.signFile(pfx, pwd, 'bin/Release', 'synergyc.exe')
			self.signFile(pfx, pwd, 'bin/Release', 'synergys.exe')
			self.signFile(pfx, pwd, 'bin/Release', 'synergyd.exe')
			self.signFile(pfx, pwd, 'bin/Release', 'synrgyhk.dll')
	
	def signFile(self, pfx, pwd, dir, file):
		self.try_chdir(dir)
		err = os.system(
			'signtool sign'
			' /f ' + pfx +
			' /p ' + pwd +
			' /t http://timestamp.verisign.com/scripts/timstamp.dll ' +
			file)
		self.restore_chdir()
	
	def runBuildCommand(self, cmd, target):
	
		self.try_chdir(self.getBuildDir(target))
		err = os.system(cmd)
		self.restore_chdir()
			
		if err != 0:
			raise Exception(cmd + ' failed: ' + str(err))
	
	def clean(self, targets=[]):
		
		# if no mode specified, use default
		if len(targets) == 0:
			targets += [self.defaultTarget,]
		
		generator = self.getGeneratorFromConfig().cmakeName

		if generator.startswith('Visual Studio'):
			# special case for version 10, use new /target:clean
			if generator.startswith('Visual Studio 10'):
				for target in targets:
					self.run_vcbuild(generator, target, '/target:clean')
				
			# any other version of visual studio, use /clean
			elif generator.startswith('Visual Studio'):
				for target in targets:
					self.run_vcbuild(generator, target, '/clean')

		else:
			cmd = ''
			if generator == "Unix Makefiles":
				print 'Cleaning with GNU Make...'
				cmd = self.make_cmd
			elif generator == 'Xcode':
				print 'Cleaning with Xcode...'
				cmd = self.xcodebuild_cmd
			else:
				raise Exception('Not supported with generator: ' + generator)

			for target in targets:
				self.try_chdir(self.getBuildDir(target))
				err = os.system(cmd + ' clean')
				self.restore_chdir()

				if err != 0:
					raise Exception('Clean failed: ' + str(err))

		# allow user to skip qui compile
		clean_targets = []
		if self.enable_make_gui:
			for target in targets:
				clean_targets.append(target + '-clean')
			
			self.make_gui(clean_targets)
	
	def make_gui(self, targets):
		if sys.platform == 'win32':
			gui_make_cmd = self.w32_make_cmd
		elif sys.platform in ['linux2', 'sunos5', 'freebsd7', 'darwin']:
			gui_make_cmd = self.make_cmd + " -w"
		else:
			raise Exception('Unsupported platform: ' + sys.platform)
		
		print 'Make GUI command: ' + gui_make_cmd
		
		if sys.platform == 'win32':
			for target in targets:
				self.try_chdir(self.gui_dir)
				err = os.system(gui_make_cmd + ' ' + target)
				self.restore_chdir()
				
				if err != 0:
					raise Exception(gui_make_cmd + ' failed with error: ' + str(err))
		else:
			self.try_chdir(self.gui_dir)
			err = os.system(gui_make_cmd)
			self.restore_chdir()

			if err != 0:
				raise Exception(gui_make_cmd + ' failed with error: ' + str(err))

			if sys.platform == 'darwin':
				self.macPostMakeGui()
	
	def macPostMakeGui(self):

		dir = self.getGenerator().binDir

		# copy synergy[cs] binaries into the bundle, since the gui
		# now looks for the binaries in the current app dir.
		shutil.copy(dir + "/synergyc",
			dir + "/Synergy.app/Contents/MacOS/")
		shutil.copy(dir + "/synergys",
			dir + "/Synergy.app/Contents/MacOS/")

		# use qt to copy libs to bundle so no dependencies are needed. do not create a
		# dmg at this point, since we need to sign it first, and then create our own
		# after signing (so that qt does not affect the signed app bundle).
		bin = "macdeployqt Synergy.app -verbose=2"
		self.try_chdir(dir)
		err = os.system(bin)
		self.restore_chdir()

		if err != 0:
			raise Exception(bin + " failed with error: " + str(err))
		
	def open(self):
		generator = self.getGeneratorFromConfig().cmakeName
		if generator.startswith('Visual Studio'):
			print 'Opening with %s...' % generator
			self.open_internal(self.sln_filepath())
			
		elif generator.startswith('Xcode'):
			print 'Opening with %s...' % generator
			self.open_internal(self.xcodeproj_filepath(), 'open')
			
		else:
			raise Exception('Not supported with generator: ' + generator)
		
	def update(self):
		print "Running Subversion update..."
		err = os.system('svn update')
		if err != 0:
			raise Exception('Could not update from repository with error code code: ' + str(err))
		
	def revision(self):
		print self.find_revision()

	def find_revision(self):
		if sys.version_info < (2, 4):
			import commands
			stdout = commands.getoutput('svn info')
		else:
			p = subprocess.Popen(['svn', 'info'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
			stdout, stderr = p.communicate()

			if p.returncode != 0:
				raise Exception('Could not get revision - svn info failed with code: ' + str(p.returncode))
		
		m = re.search('.*Revision: (\d+).*', stdout)
		if not m:
			raise Exception('Could not find revision number in svn info output.')
		
		return m.group(1)
		
	def kill(self):
		if sys.platform == 'win32':
			return os.system('taskkill /F /FI "IMAGENAME eq synergy*"')
		else:
			raise Exception('Not implemented for platform: ' + sys.platform)
		
	def doxygen(self):
		# The conf generates doc/doxygen.cfg from cmake/doxygen.cfg.in
		self.configure(self.defaultTarget, '-DCONF_DOXYGEN:BOOL=TRUE')

		err = os.system('doxygen %s/%s' % (self.doc_dir, self.doxygen_filename))
			
		if err != 0:
			raise Exception('doxygen failed with error code: ' + str(err))
				
	def dist(self, type, vcRedistDir, qtDir):

		# Package is supported by default.
		package_unsupported = False
		unixTarget = self.defaultTarget
		
		if type == '' or type == None:
			raise Exception('No type specified.')

		if type != 'win' and type != 'mac':
			self.configure(unixTarget, '-DCONF_CPACK:BOOL=TRUE')

		moveExt = ''

		if type == None:
			self.dist_usage()
			return
			
		elif type == 'src':
			if sys.platform in ['linux2', 'darwin']:
				self.distSrc()
			else:
				package_unsupported = True
			
		elif type == 'rpm':
			if sys.platform == 'linux2':
				self.dist_run('cpack -G RPM', unixTarget)
				moveExt = 'rpm'
			else:
				package_unsupported = True
			
		elif type == 'deb':
			if sys.platform == 'linux2':
				self.dist_run('cpack -G DEB', unixTarget)
				moveExt = 'deb'
			else:
				package_unsupported = True
			
		elif type == 'win':
			if sys.platform == 'win32':
				self.distNsis(vcRedistDir, qtDir)
			else:
				package_unsupported = True
			
		elif type == 'mac':
			if sys.platform == 'darwin':
				self.distMac()
			else:
				package_unsupported = True
			
		else:
			raise Exception('Package type not supported: ' + type)

		if moveExt != '':
			self.unixMove(
				self.getGenerator().buildDir + '/release/*.' + moveExt,
				self.getGenerator().binDir)

		if package_unsupported:
			raise Exception(
				("Package type, '%s' is not supported for platform, '%s'") 
				% (type, sys.platform))
		
	def distSrc(self):
		version = self.getVersionFromCmake()
		name = (self.project + '-' + version + '-Source')
		exportPath = self.getGenerator().buildDir + '/' + name

		if os.path.exists(exportPath):
			print "Removing existing export..."
			shutil.rmtree(exportPath)

		print 'Exporting repository to: ' + exportPath
		err = os.system('svn export . ' + exportPath)
		if err != 0:
			raise Exception('Repository export failed: ' + str(err))		

		packagePath = '../' + self.getGenerator().binDir + '/' + name + '.tar.gz'

		try:
			self.try_chdir(self.getGenerator().buildDir)
			print 'Packaging to: ' + packagePath
			err = os.system('tar cfvz ' + packagePath + ' ' + name)
			if err != 0:
				raise Exception('Package failed: ' + str(err))
		finally:
			self.restore_chdir()

	def unixMove(self, source, dest):
		print 'Moving ' + source + ' to ' + dest
		err = os.system('mv ' + source + ' ' + dest)
		if err != 0:
			raise Exception('Package failed: ' + str(err))
		
	def distMac(self):
		dir = self.getGenerator().binDir
		name = "Synergy"
		dist = dir + "/" + name
		
		# ensure dist dir is clean
		if os.path.exists(dist):
			os.rmdir(dist)
		
		os.makedirs(dist)
		shutil.copytree(dir + "/" + name + ".app", dist + "/" + name + ".app")

		fileName = "%s-%s-%s.dmg" % (
			self.project, 
			self.getVersionFromCmake(),
			self.getMacPackageName())
		
		cmd = "hdiutil create " + fileName + " -srcfolder ./" + name + "/ -ov"
		
		self.try_chdir(dir)
		err = os.system(cmd)
		self.restore_chdir()

	def distNsis(self, vcRedistDir, qtDir):
		
		if vcRedistDir == '':
			raise Exception(
				'VC++ redist dir path not specified (--vcredist-dir).')

		if qtDir == '':
			raise Exception(
				'QT SDK dir path not specified (--qt-dir).')

		generator = self.getGeneratorFromConfig().cmakeName

		arch = 'x86'
		installDirVar = '$PROGRAMFILES32'

		if generator.endswith('Win64'):
			arch = 'x64'
			installDirVar = '$PROGRAMFILES64'			
		
		templateFile = open(self.cmake_dir + '\Installer.nsi.in')
		template = templateFile.read()

		template = template.replace('${in:version}', self.getVersionFromCmake())
		template = template.replace('${in:arch}', arch)
		template = template.replace('${in:vcRedistDir}', vcRedistDir)
		template = template.replace('${in:qtDir}', qtDir)
		template = template.replace('${in:installDirVar}', installDirVar)

		nsiPath = self.getGenerator().buildDir + '\Installer.nsi'
		nsiFile = open(nsiPath, 'w')
		nsiFile.write(template)
		nsiFile.close()

		command = 'makensis ' + nsiPath
		print 'NSIS command: ' + command
		err = os.system(command)
		if err != 0:
			raise Exception('Package failed: ' + str(err))

	def getVersionFromCmake(self):
		cmakeFile = open('CMakeLists.txt')
		cmake = cmakeFile.read()

		majorRe = re.search('VERSION_MAJOR (\d+)', cmake)
		major = majorRe.group(1)

		minorRe = re.search('VERSION_MINOR (\d+)', cmake)
		minor = minorRe.group(1)

		revRe = re.search('VERSION_REV (\d+)', cmake)
		rev = revRe.group(1)

		return major + '.' + minor + '.' + rev

	def distftp(self, type, ftp):
		if not type:
			raise Exception('Type not specified.')
		
		if not ftp:
			raise Exception('FTP info not defined.')
		
		src = self.dist_name(type)
		dest = self.dist_name_rev(type)
		print 'Uploading %s to FTP server %s...' % (dest, ftp.host)

		srcDir = 'bin/'
		generator = self.getGeneratorFromConfig().cmakeName
		#if not generator.startswith('Visual Studio'):
		#	srcDir += 'release/'

		ftp.run(srcDir + src, dest) 
		print 'Done'
	
	def dist_name(self, type):
		ext = None
		platform = None
		
		if type == 'src':
			ext = 'tar.gz'
			platform = 'Source'
			
		elif type == 'rpm' or type == 'deb':
		
			# os_bits should be loaded with '32bit' or '64bit'
			import platform
			(os_bits, other) = platform.architecture()
		
			# get platform based on current platform
			ext = type
			if os_bits == '32bit':
				platform = 'Linux-i686'
			elif os_bits == '64bit':
				platform = 'Linux-x86_64'
			
		elif type == 'win':
			
			# get platform based on last generator used
			ext = 'exe'
			generator = self.getGeneratorFromConfig().cmakeName
			if generator.find('Win64') != -1:
				platform = 'Windows-x64'
			else:
				platform = 'Windows-x86'
			
		elif type == 'mac':
			ext = "dmg"
			platform = self.getMacPackageName()
		
		if not platform:
			raise Exception('Unable to detect package platform.')
		
		pattern = re.escape(self.project + '-') + '\d+\.\d+\.\d+' + re.escape('-' + platform + '.' + ext)
		
		# only use release dir if not windows
		target = ''

		for filename in os.listdir(self.getBinDir(target)):
			if re.search(pattern, filename):
				return filename
		
		# still here? package probably not created yet.
		raise Exception('Could not find package name with pattern: ' + pattern)
	
	def dist_name_rev(self, type):
		# find the version number (we're puting the rev in after this)
		pattern = '(.*\d+\.\d+\.\d+)(.*)'
		replace = '\g<1>-r' + self.find_revision() + '\g<2>'
		return re.sub(pattern, replace, self.dist_name(type))
	
	def dist_run(self, command, target=''):
		self.try_chdir(self.getBuildDir(target))
		print 'CPack command: ' + command
		err = os.system(command)
		self.restore_chdir()
		if err != 0:
			raise Exception('Package failed: ' + str(err))

	def dist_usage(self):
		print ('Usage: %s package [package-type]\n'
			'\n'
			'Replace [package-type] with one of:\n'
			'  src    .tar.gz source (Posix only)\n'
			'  rpm    .rpm package (Red Hat)\n'
			'  deb    .deb paclage (Debian)\n'
			'  win    .exe installer (Windows)\n'
			'  mac    .dmg package (Mac OS X)\n'
			'\n'
			'Example: %s package src-tgz') % (self.this_cmd, self.this_cmd)

	def about(self):
		print ('Help Me script, from the Synergy project.\n'
			'%s\n'
			'\n'
			'For help, run: %s help') % (self.website_url, self.this_cmd)

	def try_chdir(self, dir):
		global prevdir

		if dir == '':
			prevdir = ''
			return

		# Ensure temp build dir exists.
		if not os.path.exists(dir):
			print 'Creating dir: ' + dir
			os.makedirs(dir)
 
		prevdir = os.path.abspath(os.curdir)

		# It will exist by this point, so it's safe to chdir.
		print 'Entering dir: ' + dir
		os.chdir(dir)

	def restore_chdir(self):
		global prevdir
		if prevdir == '':
			return
		print 'Going back to: ' + prevdir
		os.chdir(prevdir)

	def open_internal(self, project_filename, application = ''):

		if not os.path.exists(project_filename):
			raise Exception('Project file (%s) not found, run hm conf first.' % project_filename)
		else:
			path = project_filename
			
			if application != '':
				path = application + ' ' + path
			
			err = os.system(path)
			if err != 0:
				raise Exception('Could not open project with error code code: ' + str(err))

	def setup(self, target=''):
		print "Running setup..."

		oldGenerator = self.findGeneratorFromConfig()
		if not oldGenerator == None:
			for target in ['debug', 'release']:				
				buildDir = oldGenerator.getBuildDir(target)

				cmakeCacheFilename = 'CMakeCache.txt'
				if buildDir != '':
					cmakeCacheFilename = buildDir + '/' + cmakeCacheFilename

				if os.path.exists(cmakeCacheFilename):
					print "Removing %s, since generator changed." % cmakeCacheFilename
					os.remove(cmakeCacheFilename)

		# always either get generator from args, or prompt user when 
		# running setup
		generator = self.get_generator_from_prompt()

		config = self.getConfig()
		config.set('hm', 'setup_version', self.setup_version)
		
		# store the generator so we don't need to ask again
		config.set('cmake', 'generator', generator)

		self.write_config(config)

		# for all targets, set conf not run
		self.setConfRun('all', False)
		self.setConfRun('debug', False)
		self.setConfRun('release', False)

		print "Setup complete."

	def getConfig(self):
		if os.path.exists(self.configFilename):
			config = ConfigParser.ConfigParser()
			config.read(self.configFilename)
		else:
			config = ConfigParser.ConfigParser()

		if not config.has_section('hm'):
			config.add_section('hm')

		if not config.has_section('cmake'):
			config.add_section('cmake')

		return config

	def write_config(self, config, target=''):
		if not os.path.isdir(self.configDir):
			os.mkdir(self.configDir)
		configfile = open(self.configFilename, 'wb')
		config.write(configfile)

	def getGeneratorFromConfig(self):
		generator = self.findGeneratorFromConfig()
		if generator:
			return generator
		
		raise Exception("Could not find generator: " + name)

	def findGeneratorFromConfig(self):
		config = ConfigParser.RawConfigParser()
		config.read(self.configFilename)
		
		if not config.has_section('cmake'):
			return None
		
		name = config.get('cmake', 'generator')

		generators = self.get_generators()
		keys = generators.keys()
		keys.sort()
		for k in keys:
			if generators[k].cmakeName == name:
				return generators[k]
		
		return None

	def min_setup_version(self, version):
		if os.path.exists(self.configFilename):
			config = ConfigParser.RawConfigParser()
			config.read(self.configFilename)

			try:
				return config.getint('hm', 'setup_version') >= version
			except:
				return False
		else:
			return False

	def hasConfRun(self, target):
		if self.min_setup_version(2):
			config = ConfigParser.RawConfigParser()
			config.read(self.configFilename)
			try:
				return config.getboolean('hm', 'conf_done_' + target)
			except:
				return False
		else:
			return False

	def setConfRun(self, target, hasRun=True):
		if self.min_setup_version(3):
			config = ConfigParser.RawConfigParser()
			config.read(self.configFilename)
			config.set('hm', 'conf_done_' + target, hasRun)
			self.write_config(config)
		else:
			raise Exception("User does not have correct setup version.")

	def get_generators(self):
		if sys.platform == 'win32':
			return self.win32_generators
		elif sys.platform in ['linux2', 'sunos5', 'freebsd7', 'aix5']:
			return self.unix_generators
		elif sys.platform == 'darwin':
			return self.darwin_generators
		else:
			raise Exception('Unsupported platform: ' + sys.platform)
			
	def get_generator_from_prompt(self):
		return self.getGenerator().cmakeName

	def getGenerator(self):
		generators = self.get_generators()
		if len(generators.keys()) == 1:
			return generators[generators.keys()[0]]
		
		# if user has specified a generator as an argument
		if self.generator_id:
			return generators[int(self.generator_id)]

		conf = self.findGeneratorFromConfig()
		if conf:
			return conf
		
		raise Exception(
			'Generator not specified, use -g arg ' + 
			'(use `hm genlist` for a list of generators).')

	def setup_generator_prompt(self, generators):

		if self.no_prompts:
			raise Exception('User prompting is disabled.')
	
		prompt = 'Enter a number:'
		print prompt,
		
		generator_id = raw_input()
		
		if generator_id in generators:
			print 'Selected generator:', generators[generator_id]
		else:
			print 'Invalid number, try again.'
			self.setup_generator_prompt(generators)

		return generators[generator_id]

	def get_vcvarsall(self, generator):	
		import platform, _winreg
		
		# os_bits should be loaded with '32bit' or '64bit'
		(os_bits, other) = platform.architecture()
		
		# visual studio is a 32-bit app, so when we're on 64-bit, we need to check the WoW dungeon
		if os_bits == '64bit':
			key_name = r'SOFTWARE\Wow6432Node\Microsoft\VisualStudio\SxS\VS7'
		else:
			key_name = r'SOFTWARE\Microsoft\VisualStudio\SxS\VC7'
		
		try:
			key = _winreg.OpenKey(_winreg.HKEY_LOCAL_MACHINE, key_name)
		except:
			raise Exception('Unable to open Visual Studio registry key. Application may not be installed.')
		
		if generator.startswith('Visual Studio 8'):
			value,type = _winreg.QueryValueEx(key, '8.0')
		elif generator.startswith('Visual Studio 9'):
			value,type = _winreg.QueryValueEx(key, '9.0')
		elif generator.startswith('Visual Studio 10'):
			value,type = _winreg.QueryValueEx(key, '10.0')
		else:
			raise Exception('Cannot determine vcvarsall.bat location for: ' + generator)
		
		# not sure why, but the value on 64-bit differs slightly to the original
		if os_bits == '64bit':
			path = value + r'vc\vcvarsall.bat'
		else:
			path = value + r'vcvarsall.bat'		
		
		if not os.path.exists(path):
			raise Exception("'%s' not found." % path)
		
		return path

	def run_vcbuild(self, generator, mode, args=''):
		import platform
		
		# os_bits should be loaded with '32bit' or '64bit'
		(os_bits, other) = platform.architecture()
		# Now we choose the parameters bases on OS 32/64 and our target 32/64
		# http://msdn.microsoft.com/en-us/library/x4d2c09s%28VS.80%29.aspx
		
		# valid options are only: ia64  amd64 x86_amd64 x86_ia64 
		# but calling vcvarsall.bat does not garantee that it will work
		# ret code from vcvarsall.bat is always 0 so the only way of knowing that I worked is by analysing the text output
		# ms bugg: install VS9, FeaturePack, VS9SP1 and you'll obtain a vcvarsall.bat that fails.
		if generator.find('Win64') != -1:
			# target = 64bit
			if os_bits == '32bit':
				vcvars_platform = 'x86_amd64' # 32bit OS building 64bit app
			else:
				vcvars_platform = 'amd64'  # 64bit OS building 64bit app
			config_platform = 'x64'
		else: # target = 32bit
			vcvars_platform = 'x86' # 32/64bit OS building 32bit app
			config_platform = 'Win32'
		if mode == 'release':
			config = 'Release'
		else:
			config = 'Debug'
				
		if generator.startswith('Visual Studio 10'):
			cmd = ('@echo off\n'
				'call "%s" %s \n'
				'msbuild /nologo %s /p:Configuration="%s" /p:Platform="%s" "%s"'
				) % (self.get_vcvarsall(generator), vcvars_platform, args, config, config_platform, self.sln_filepath())
		else:
			config = config + '|' + config_platform
			cmd = ('@echo off\n'
				'call "%s" %s \n'
				'vcbuild /nologo %s "%s" "%s"'
				) % (self.get_vcvarsall(generator), vcvars_platform, args, self.sln_filepath(), config)
		
		# Generate a batch file, since we can't use environment variables directly.
		temp_bat = self.getBuildDir() + r'\vcbuild.bat'
		file = open(temp_bat, 'w')
		file.write(cmd)
		file.close()

		err = os.system(temp_bat)
		if err != 0:
			raise Exception('Microsoft compiler failed with error code: ' + str(err))

	def ensure_setup_latest(self):
		if not self.min_setup_version(self.setup_version):
			self.setup()

	def reformat(self):
		err = os.system(
			r'tool\astyle\AStyle.exe '
			'--quiet --suffix=none --style=java --indent=force-tab=4 --recursive '
			'lib/*.cpp lib/*.h cmd/*.cpp cmd/*.h')
			
		if err != 0:
			raise Exception('Reformat failed with error code: ' + str(err))

	def printGeneratorList(self):
		generators = self.get_generators()
		keys = generators.keys()
		keys.sort()
		for k in keys:
			print str(k) + ': ' + generators[k].cmakeName

	def getMacVersion(self):
		# if we've built with an older sdk, then use that as the
		# os version for package names, etc.
		config = self.getConfig()
		if config.has_option("cmake", "mac_sdk"):
			macSdk = config.get("cmake", "mac_sdk")
			split = macSdk.split('.')
			major = int(split[0])
			minor = int(split[1])
			return (major, minor)
			
		import commands
		versions = commands.getoutput('/usr/bin/sw_vers')
		result = re.search('ProductVersion:\t(\d+)\.(\d+)', versions)

		if not result:
			print versions
			raise Exception(
				'Could not find Mac OS X version in sw_vers output.')

		major = int(result.group(1))
		minor = int(result.group(2))
		return (major, minor)

	def getMacPackageName(self):

		(major, minor) = self.getMacVersion()

		if major == 10:
			if minor <= 4:
				# 10.4: intel and power pc
				arch = "Universal"
			elif minor <= 6:
				# 10.5: 32-bit intel
				arch = "i386"
			else:
				# 10.7: 64-bit intel (gui only)
				arch = "x86_64"
		else:
			raise Exception("Mac OS major version unknown: " +
					str(major))

		# version is major and minor with no dots (e.g. 106)
		version = str(major) + str(minor)

		return "MacOSX%s-%s" % (version, arch)

	def reset(self):
		if os.path.exists('build'):
			shutil.rmtree('build')
		if os.path.exists('bin'):
			shutil.rmtree('bin')
		if os.path.exists('lib'):
			shutil.rmtree('lib')
		if os.path.exists('src/gui/tmp'):
			shutil.rmtree('src/gui/tmp')

# the command handler should be called only from hm.py (i.e. directly 
# from the command prompt). the purpose of this class is so that we 
# don't need to do argument handling all over the place in the internal
# commands class.
class CommandHandler:
	ic = InternalCommands()
	build_targets = []
	vcRedistDir = ''
	qtDir = ''
	
	def __init__(self, argv, opts, args, verbose):
		
		self.ic.verbose = verbose
		
		self.opts = opts
		self.args = args
		
		for o, a in self.opts:
			if o == '--no-prompts':
				self.ic.no_prompts = True
			elif o in ('-g', '--generator'):
				self.ic.generator_id = a
			elif o == '--make-gui':
				self.ic.enable_make_gui = True
			elif o in ('-d', '--debug'):
				self.build_targets += ['debug',]
			elif o in ('-r', '--release'):
				self.build_targets += ['release',]
			elif o == '--vcredist-dir':
				self.vcRedistDir = a
			elif o == '--qt-dir':
				self.qtDir = a
			elif o == '--game-device':
				self.ic.gameDevice = True
			elif o == '--vnc':
				self.ic.vncSupport = True
			elif o == '--mac-sdk':
				self.ic.macSdk = a
	
	def about(self):
		self.ic.about()
	
	def setup(self):
		self.ic.setup()
	
	def configure(self):
		self.ic.configureAll(self.build_targets)
	
	def build(self):
		self.ic.build(self.build_targets)
	
	def clean(self):
		self.ic.clean(self.build_targets)
	
	def update(self):
		self.ic.update()
	
	def install(self):
		print 'Not yet implemented: install'
	
	def doxygen(self):
		self.ic.doxygen ()
	
	def dist(self):
		
		type = None
		if len(self.args) > 0:
			type = self.args[0]		
				
		self.ic.dist(type, self.vcRedistDir, self.qtDir)

	def distftp(self):
		type = None
		host = None
		user = None
		password = None
		dir = None
		
		if len(self.args) > 0:
			type = self.args[0]
		
		for o, a in self.opts:
			if o == '--host':
				host = a
			elif o == '--user':
				user = a
			elif o == '--pass':
				password = a
			elif o == '--dir':
				dir = a
		
		ftp = None
		if host:
			ftp = ftputil.FtpUploader(
				host, user, password, dir)
		
		self.ic.distftp(type, ftp)
	
	def destroy(self):
		self.ic.destroy()
	
	def kill(self):
		self.ic.kill()
	
	def usage(self):
		self.ic.usage()
	
	def revision(self):
		self.ic.revision()
	
	def reformat(self):
		self.ic.reformat()
	
	def open(self):
		self.ic.open()

	def genlist(self):
		self.ic.printGeneratorList()

	def reset(self):
		self.ic.reset()
		
	def signwin(self):
		pfx = None
		pwd = None
		dist = False
		for o, a in self.opts:
			if o == '--pfx':
				pfx = a
			elif o == '--pwd':
				pwd = a
			elif o == '--dist':
				dist = True
		self.ic.signwin(pfx, pwd, dist)

	def signmac(self):
		idenity = None
		for o, a in self.opts:
			if o == '--identity':
				identity = a
		
		# HACK: codesign fails intermittently. we need some sort of retry mechanism
		# but i don't have time right now so just hammer the crap out of it and hope
		# one attempt works.
		self.ic.signmac(identity)
		self.ic.signmac(identity)
		self.ic.signmac(identity)
		self.ic.signmac(identity)
		self.ic.signmac(identity)

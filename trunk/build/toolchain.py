# synergy -- mouse and keyboard sharing utility
# Copyright (C) 2009 Chris Schoeneman, Nick Bolton, Sorin Sbarnea
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

if sys.version_info >= (2, 4):
	import subprocess

class InternalCommands:
	
	project = 'synergy'
	setup_version = 4 # increment to force setup/config
	website_url = 'http://synergy-foss.org/'

	this_cmd = 'hm'
	cmake_cmd = 'cmake'
	qmake_cmd = 'qmake'
	make_cmd = 'make'
	xcodebuild_cmd = 'xcodebuild'
	w32_make_cmd = 'mingw32-make'
	w32_qt_version = '4.6.2'

	source_dir = '..' # Source, relative to build.
	cmake_dir = 'cmake'
	_bin_dir = 'bin'
	gui_dir = 'gui'
	doc_dir = 'doc'

	sln_filename = '%s.sln' % project
	xcodeproj_filename = '%s.xcodeproj' % project
	config_filename = '%s.cfg' % this_cmd
	qtpro_filename = 'qsynergy.pro'
	doxygen_filename = 'doxygen.cfg'
	
	macZipFiles = [
		'synergyc', 'synergys',
		'../../doc/synergy.conf.example',
		'../../doc/MacReadme.txt']

	cmake_url = 'http://www.cmake.org/cmake/resources/software.html'

	# try_chdir(...) and restore_chdir() will use this
	prevdir = ''
	
	# by default, no index specified as arg
	generator_id = None
	
	# by default, prompt user for input
	no_prompts = False
	
	# by default, don't compile the gui
	enable_make_gui = False

	win32_generators = {
		1 : 'Visual Studio 10',
		2 : 'Visual Studio 10 Win64',
		3 : 'Visual Studio 9 2008',
		4 : 'Visual Studio 9 2008 Win64',
		5 : 'Visual Studio 8 2005',
		6 : 'Visual Studio 8 2005 Win64',
	}

	unix_generators = {
		1 : 'Unix Makefiles',
	}

	darwin_generators = {
		1 : 'Unix Makefiles',
		2 : 'Xcode',
	}

	def getBinDir(self, target=''):
		workingDir = self._bin_dir
		if target != '':
			workingDir += '/' + target
		return workingDir

	def config_filepath(self, target=''):
		return '%s/%s' % (self.getBinDir(target), self.config_filename)

	def sln_filepath(self):
		return '%s\%s' % (self.getBinDir(), self.sln_filename)

	def xcodeproj_filepath(self, target=''):
		return '%s/%s' % (self.getBinDir(target), self.xcodeproj_filename)
		
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
	
	def configure(self, target):
		self.configure_internal(target)
		
		print ('Configure complete!\n\n'
			'Open project now: %s open\n'
			'Command line build: %s build'
			) % (self.this_cmd, self.this_cmd)

	def configure_internal(self, target='', extraArgs=''):
		
		cmake_args = ''

		# ensure latest setup and do not ask config for generator (only fall 
		# back to prompt if not specified as arg)
		self.ensure_setup_latest()
		
		# ensure that we have access to cmake
		_cmake_cmd = self.persist_cmake()
		
		# now that we know we've got the latest setup, we can ask the config
		# file for the generator (but again, we only fall back to this if not 
		# specified as arg).
		generator = self.get_generator_from_config()
		
		if generator != '':
			cmake_args += ' -G "' + generator + '"'
		
		# for non-vs always specify a build type (debug, release, etc)
		if not generator.startswith('Visual Studio'):
			# default is default for non-vs
			if target == '':
				target = 'debug'
			cmake_args += ' -DCMAKE_BUILD_TYPE=' + target.capitalize()
		
		# if not visual studio, use parent dir
		sourceDir = self.source_dir
		if not generator.startswith('Visual Studio'):
			sourceDir += '/..'
		
		if extraArgs != '':
			cmake_args += ' ' + extraArgs

		cmake_cmd_string = _cmake_cmd + cmake_args + ' ' + sourceDir
		
		print "CMake command: " + cmake_cmd_string
		
		# Run from build dir so we have an out-of-source build.
		self.try_chdir(self.getBinDir(target))
		err = os.system(cmake_cmd_string)
		self.restore_chdir()

		if err != 0:
			raise Exception('CMake encountered error: ' + str(err))
		
		# allow user to skip qui compile
		if self.enable_make_gui:
			
			# make sure we have qmake
			self.persist_qmake()
			
			qmake_cmd_string = self.qmake_cmd + ' ' + self.qtpro_filename
			print "QMake command: " + qmake_cmd_string
			
			# run qmake from the gui dir
			self.try_chdir(self.gui_dir)
			err = os.system(qmake_cmd_string)
			self.restore_chdir()
			
			if err != 0:
				raise Exception('QMake encountered error: ' + str(err))
		
		self.set_conf_run()

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
		if sys.platform == 'win32':
			self.persist_w32_make()

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

	def persist_w32_make():
		# TODO
		pass

	def build(self, targets=[], skipConfig=False):

		# if no mode specified, default to debug
		if len(targets) == 0:
			targets += ['debug',]
	
		self.ensure_setup_latest()

		generator = self.get_generator_from_config()

		if generator.startswith('Visual Studio'):
			
			# only need to configure once for vs
			if not self.has_conf_run() and not skipConfig:
				self.configure_internal()
		
			for target in targets:
				self.run_vcbuild(generator, target)
		
		else:
			
			cmd = ''
			if generator == "Unix Makefiles":
				print 'Building with GNU Make...'
				cmd = self.make_cmd
			elif generator == 'Xcode':
				print 'Building with Xcode...'
				cmd = self.xcodebuild_cmd
			else:
				raise Exception('Not supported with generator: ' + generator)

			for target in targets:
					
				if not self.has_conf_run(target) and not skipConfig:
					self.configure_internal(target)
					
				self.try_chdir(self.getBinDir(target))
				err = os.system(cmd)
				self.restore_chdir()
				
				if err != 0:
					raise Exception(cmd + ' failed: ' + str(err))

		# allow user to skip qui compile
		if self.enable_make_gui:
			self.make_gui(targets)
	
	def clean(self, targets=[]):
		
		# if no mode specified, default to debug
		if len(targets) == 0:
			targets += ['debug',]
		
		generator = self.get_generator_from_config()

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
				self.try_chdir(self.getBinDir(target))
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
		elif sys.platform in ['linux2', 'sunos5', 'freebsd7']:
			gui_make_cmd = self.make_cmd
		elif sys.platform == 'darwin':
			gui_make_cmd = self.xcodebuild_cmd
		else:
			raise Exception('Unsupported platform: ' + sys.platform)
		
		print 'Make GUI command: ' + gui_make_cmd
		
		# HACK: don't know how to build in either debug or release on unix; 
		# always builds release!
		if sys.platform == 'win32':
			for target in targets:
				self.try_chdir(self.gui_dir)
				err = os.system(gui_make_cmd + ' ' + target)
				self.restore_chdir()
				
				if err != 0:
					raise Exception(gui_make_cmd + ' failed with error: ' + str(err))
		else:
			if sys.platform == 'darwin':
				make_dir = self.gui_dir + '/QSynergy.xcodeproj'
			else:
				make_dir = self.gui_dir

			self.try_chdir(make_dir)
			err = os.system(gui_make_cmd)
			self.restore_chdir()
	
	def open(self):
		generator = self.get_generator_from_config()
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
		if not self.has_conf_run():
			self.configure_internal()

		err = os.system('doxygen %s/%s' % (self.doc_dir, self.doxygen_filename))
			
		if err != 0:
			raise Exception('doxygen failed with error code: ' + str(err))
				
	def dist(self, type, vcRedistDir, qtDir):

		# Package is supported by default.
		package_unsupported = False
		unixTarget = 'release'
		
		if type != 'win':
			self.configure_internal(unixTarget, '-DCONF_CPACK:BOOL=TRUE')

		# make sure we have a release build to package
		self.build(['release'], skipConfig=True)

		if type == None:
			self.dist_usage()
			return
			
		elif type == 'src':
			if sys.platform in ['linux2', 'darwin']:
				self.dist_run('make package_source', unixTarget)
			else:
				package_unsupported = True
			
		elif type == 'rpm':
			if sys.platform == 'linux2':
				self.dist_run('cpack -G RPM', unixTarget)
			else:
				package_unsupported = True
			
		elif type == 'deb':
			if sys.platform == 'linux2':
				self.dist_run('cpack -G DEB', unixTarget)
			else:
				package_unsupported = True
			
		elif type == 'win':
			if sys.platform == 'win32':
				self.distNsis(vcRedistDir, qtDir)
			else:
				package_unsupported = True
			
		elif type == 'mac':
			if sys.platform == 'darwin':
				self.distMac(unixTarget)
			else:
				package_unsupported = True
			
		else:
			raise Exception('Package type not supported: ' + type)

		if package_unsupported:
			raise Exception(
				("Package type, '%s' is not supported for platform, '%s'") 
				% (type, sys.platform))
		

	def distMac(self, unixTarget):
		# nb: disabling package maker, as it doesn't
		# work too well (screws with permissions and causes boot to fail).
		#self.dist_run('cpack -G PackageMaker', unixTarget)

		version = self.getVersionFromCmake()
		zipFile = (self.project + '-' + version + '-' +
				   self.getMacPackageName() + '.zip')

		# nb: temporary fix (just distribute a zip)
		bin = self.getBinDir(unixTarget)
		self.try_chdir(bin)

		try:
			for f in self.macZipFiles:
				if not os.path.exists(f):
					raise Exception('File does not exist: ' + f)

			zipCmd = ('zip ' + zipFile + ' ' + ' '.join(self.macZipFiles));
			
			print 'Creating package: ' + zipCmd
			err = os.system(zipCmd)
			if err != 0:
				raise Exception('Zip failed, code: ' + err)
			
		finally:
			self.restore_chdir()

	def distNsis(self, vcRedistDir, qtDir):
		
		if vcRedistDir == '':
			raise Exception(
				'VC++ redist dir path not specified (--vcredist-dir).')

		if qtDir == '':
			raise Exception(
				'QT SDK dir path not specified (--qt-dir).')

		generator = self.get_generator_from_config()

		arch = 'x86'
		installDirVar = '$PROGRAMFILES32'

		if generator.endswith('Win64'):
			arch = 'x64'
			installDirVar = '$PROGRAMFILES64'			
		
		templateFile = open('cmake\Installer.nsi.in')
		template = templateFile.read()

		template = template.replace('${in:version}', self.getVersionFromCmake())
		template = template.replace('${in:arch}', arch)
		template = template.replace('${in:vcRedistDir}', vcRedistDir)
		template = template.replace('${in:qtDir}', qtDir)
		template = template.replace('${in:installDirVar}', installDirVar)

		nsiPath = 'bin\Installer.nsi'
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
		generator = self.get_generator_from_config()
		if not generator.startswith('Visual Studio'):
			srcDir += 'release/'

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
			generator = self.get_generator_from_config()
			if generator.find('Win64') != -1:
				platform = 'Windows-x64'
			else:
				platform = 'Windows-x86'
			
		elif type == 'mac':
			#ext = 'dmg'
			ext = 'zip'
			platform = self.getMacPackageName()
		
		if not platform:
			raise Exception('Unable to detect package platform.')
		
		pattern = re.escape(self.project + '-') + '\d\.\d\.\d' + re.escape('-' + platform + '.' + ext)
		
		# only use release dir if not windows
		target = ''
		if type != 'win':
			target = 'release'

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
		self.try_chdir(self.getBinDir(target))
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

		# Ensure temp build dir exists.
		if not os.path.exists(dir):
			print 'Creating dir: ' + dir
			os.mkdir(dir)

		global prevdir    
		prevdir = os.path.abspath(os.curdir)

		# It will exist by this point, so it's safe to chdir.
		print 'Entering dir: ' + dir
		os.chdir(dir)

	def restore_chdir(self):
		global prevdir
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

		# always either get generator from args, or prompt user when 
		# running setup
		generator = self.get_generator_from_prompt()

		# Create build dir, since config file resides there.
		if not os.path.exists(self.getBinDir(target)):
			os.mkdir(self.getBinDir(target))

		if os.path.exists(self.config_filepath()):
			config = ConfigParser.ConfigParser()
			config.read(self.config_filepath())
		else:
			config = ConfigParser.ConfigParser()

		if not config.has_section('hm'):
			config.add_section('hm')

		if not config.has_section('cmake'):
			config.add_section('cmake')
		
		config.set('hm', 'setup_version', self.setup_version)
		
		# store the generator so we don't need to ask again
		config.set('cmake', 'generator', generator)

		self.write_config(config)

		cmakecache_filename = '%s/CMakeCache.txt' % self.getBinDir(target)
		if os.path.exists(cmakecache_filename):
			print "Removing %s, since generator changed." % cmakecache_filename
			os.remove(cmakecache_filename)

		print "\nSetup complete."

	def write_config(self, config, target=''):
		configfile = open(self.config_filepath(target), 'wb')
		config.write(configfile)

	def get_generator_from_config(self):
		if self.generator_id:
			return self.getGenerator()
		else:
			config = ConfigParser.RawConfigParser()
			config.read(self.config_filepath())
			return config.get('cmake', 'generator')

	def min_setup_version(self, version):
		if os.path.exists(self.config_filepath()):
			config = ConfigParser.RawConfigParser()
			config.read(self.config_filepath())

			try:
				return config.getint('hm', 'setup_version') >= version
			except:
				return False
		else:
			return False

	def has_conf_run(self, target=''):
		if self.min_setup_version(2):
			config = ConfigParser.RawConfigParser()
			config.read(self.config_filepath(target))
			try:
				return config.getboolean('hm', 'has_conf_run')
			except:
				return False
		else:
			return False

	def set_conf_run(self):
		if self.min_setup_version(3):
			config = ConfigParser.RawConfigParser()
			config.read(self.config_filepath())
			config.set('hm', 'has_conf_run', True)
			self.write_config(config)
		else:
			raise Exception("User does not have correct setup version.")

	def get_generators(self):
		if sys.platform == 'win32':
			return self.win32_generators
		elif sys.platform in ['linux2', 'sunos5', 'freebsd7']:
			return self.unix_generators
		elif sys.platform == 'darwin':
			return self.darwin_generators
		else:
			raise Exception('Unsupported platform: ' + sys.platform)
			
	def get_generator_from_prompt(self):
		return self.getGenerator()

	def getGenerator(self):
		generators = self.get_generators()
		if len(generators.keys()) == 1:
			return generators[generators.keys()[0]]
		
		# if user has specified a generator as an argument
		if self.generator_id:
			return generators[int(self.generator_id)]
		else:
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
			raise Exception('Cannot determin vcvarsall.bat location for: ' + generator)
		
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
		temp_bat = self.getBinDir() + r'\vcbuild.bat'
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
			print str(k) + ': ' + generators[k]

	def getMacPackageName(self):
		import commands
		versions = commands.getoutput('/usr/bin/sw_vers')
		result = re.search('ProductVersion:\t(\d+)\.(\d+)', versions)

		if not result:
			print versions
			raise Exception(
				'Could not find Mac OS X version in sw_vers output.')

		# version is major and minor with no dots (e.g. 106)
		return ('MacOSX' + str(result.group(1)) +
				str(result.group(2)) + '-Universal');

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
	
	def about(self):
		self.ic.about()
	
	def setup(self):
		self.ic.setup()
	
	def configure(self):
		target = ''
		if (len(self.build_targets) > 0):
			target = self.build_targets[0]
		self.ic.configure(target)
	
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
	
	def hammer(self):
		self.ic.hammer()
	
	def reformat(self):
		self.ic.reformat()
	
	def open(self):
		self.ic.open()

	def genlist(self):
		self.ic.printGeneratorList()

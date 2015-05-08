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

# TODO: split this file up, it's too long!

import sys, os, ConfigParser, shutil, re, ftputil, zipfile, glob, commands
from generators import Generator, EclipseGenerator, XcodeGenerator, MakefilesGenerator
from getopt import gnu_getopt

if sys.version_info >= (2, 4):
	import subprocess

class Toolchain:
	
	# minimum required version.
	# 2.6 needed for ZipFile.extractall.
	# do not change to 2.7, as the build machines are still at 2.6
	# and are a massive pain in the ass to upgrade.
	requiredMajor = 2
	requiredMinor = 6

	# options used by all commands
	globalOptions = 'v'
	globalOptionsLong = ['no-prompts', 'verbose', 'skip-gui', 'skip-core']

	# list of valid commands as keys. the values are optarg strings, but most 
	# are None for now (this is mainly for extensibility)
	cmd_opt_dict = {
		'about'     : ['', []],
		'setup'     : ['g:', ['generator=']],
		'configure' : ['g:dr', ['generator=', 'debug', 'release', 'mac-sdk=', 'mac-identity=']],
		'build'     : ['dr', ['debug', 'release']],
		'clean'     : ['dr', ['debug', 'release']],
		'update'    : ['', []],
		'install'   : ['', []],
		'doxygen'   : ['', []],
		'dist'      : ['', ['vcredist-dir=', 'qt-dir=']],
		'distftp'   : ['', ['host=', 'user=', 'pass=', 'dir=']],
		'kill'      : ['', []],
		'usage'     : ['', []],
		'revision'  : ['', []],
		'reformat'  : ['', []],
		'open'      : ['', []],
		'genlist'   : ['', []],
		'reset'	    : ['', []],
		'signwin'	: ['', ['pfx=', 'pwd=', 'dist']],
		'signmac'	: ['', []]
	}

	# aliases to valid commands
	cmd_alias_dict = {
		'info'	    : 'about',
		'help'      : 'usage',
		'package'   : 'dist',
		'docs'      : 'doxygen',
		'make'      : 'build',
		'cmake'     : 'configure',
	}

	def complete_command(self, arg):
		completions = []
		
		for cmd, optarg in self.cmd_opt_dict.iteritems():
			# if command was matched fully, return only this, so that
			# if `dist` is typed, it will return only `dist` and not
			# `dist` and `distftp` for example.
			if cmd == arg:
				return [cmd,]
			if cmd.startswith(arg):
				completions.append(cmd)
		
		for alias, cmd in self.cmd_alias_dict.iteritems():
			# don't know if this will work just like above, but it's
			# probably worth adding.
			if alias == arg:
				return [alias,]
			if alias.startswith(arg):
				completions.append(alias)
		
		return completions

	def start_cmd(self, argv):
		
		cmd_arg = ''
		if len(argv) > 1:
			cmd_arg = argv[1]
				
		# change common help args to help command
		if cmd_arg in ('--help', '-h', '--usage', '-u', '/?'):
			cmd_arg = 'usage'

		completions = self.complete_command(cmd_arg)
		
		if cmd_arg and len(completions) > 0:

			if len(completions) == 1:

				# get the only completion (since in this case we have 1)
				cmd = completions[0]

				# build up the first part of the map (for illustrative purposes)
				cmd_map = list()
				if cmd_arg != cmd:
					cmd_map.append(cmd_arg)
					cmd_map.append(cmd)
				
				# map an alias to the command, and build up the map
				if cmd in self.cmd_alias_dict.keys():
					alias = cmd
					if cmd_arg == cmd:
						cmd_map.append(alias)
					cmd = self.cmd_alias_dict[cmd]
					cmd_map.append(cmd)
				
				# show command map to avoid confusion
				if len(cmd_map) != 0:
					print 'Mapping command: %s' % ' -> '.join(cmd_map)
				
				self.run_cmd(cmd, argv[2:])
				
				return 0
				
			else:
				print (
					'Command `%s` too ambiguous, '
					'could mean any of: %s'
					) % (cmd_arg, ', '.join(completions))
		else:

			if len(argv) == 1:
				print 'No command specified, showing usage.\n'
			else:
				print 'Command not recognised: %s\n' % cmd_arg
			
			self.run_cmd('usage')
		
		# generic error code if not returned sooner
		return 1

	def run_cmd(self, cmd, argv = []):
		
		verbose = False
		try:
			options_pair = self.cmd_opt_dict[cmd]
			
			options = self.globalOptions + options_pair[0]
			
			options_long = []
			options_long.extend(self.globalOptionsLong)
			options_long.extend(options_pair[1])
			
			opts, args = gnu_getopt(argv, options, options_long)
			
			for o, a in opts:
				if o in ('-v', '--verbose'):
					verbose = True
			
			# pass args and optarg data to command handler, which figures out
			# how to handle the arguments
			handler = CommandHandler(argv, opts, args, verbose)
			
			# use reflection to get the function pointer
			cmd_func = getattr(handler, cmd)
		
			cmd_func()
		except:
			if not verbose:
				# print friendly error for users
				sys.stderr.write('Error: ' + sys.exc_info()[1].__str__() + '\n')
				sys.exit(1)
			else:
				# if user wants to be verbose let python do it's thing
				raise

	def run(self, argv):
		if sys.version_info < (self.requiredMajor, self.requiredMinor):
			print ('Python version must be at least ' +
					 str(self.requiredMajor) + '.' + str(self.requiredMinor) + ', but is ' +
					 str(sys.version_info[0]) + '.' + str(sys.version_info[1]))
			sys.exit(1)

		try:
			self.start_cmd(argv)
		except KeyboardInterrupt:
			print '\n\nUser aborted, exiting.'

class InternalCommands:
	
	project = 'synergy'
	setup_version = 5 # increment to force setup/config
	website_url = 'http://synergy-project.org/'

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
	extDir = 'ext'

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
	
	# by default, compile the core
	enableMakeCore = True
	
	# by default, compile the gui
	enableMakeGui = True
	
	# by default, unknown
	macSdk = None
	
	# by default, unknown
	macIdentity = None
	
	# gtest dir with version number
	gtestDir = 'gtest-1.6.0'
	
	# gmock dir with version number
	gmockDir = 'gmock-1.6.0'

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
		2 : XcodeGenerator(),
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
			'  genlist     Shows the list of available platform generators\n'
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

	def checkGTest(self):
    
		dir = self.extDir + '/' + self.gtestDir
		if (os.path.isdir(dir)):
			return
		
		zipFilename = dir + '.zip'
		if (not os.path.exists(zipFilename)):
			raise Exception('GTest zip not found at: ' + zipFilename)
		
		if not os.path.exists(dir):
			os.mkdir(dir)
		
		zip = zipfile.ZipFile(zipFilename)
		self.zipExtractAll(zip, dir)

	def checkGMock(self):
    
		dir = self.extDir + '/' + self.gmockDir
		if (os.path.isdir(dir)):
			return
		
		zipFilename = dir + '.zip'
		if (not os.path.exists(zipFilename)):
			raise Exception('GMock zip not found at: ' + zipFilename)
		
		if not os.path.exists(dir):
			os.mkdir(dir)
		
		zip = zipfile.ZipFile(zipFilename)
		self.zipExtractAll(zip, dir)

	# ZipFile.extractall() is buggy in 2.6.1
	# http://bugs.python.org/issue4710
	def zipExtractAll(self, z, dir):
		if not dir.endswith("/"):
			dir += "/"
		
		for f in z.namelist():
			if f.endswith("/"):
				os.makedirs(dir + f)
			else:
				z.extract(f, dir)

	def configure(self, target='', extraArgs=''):

		# ensure latest setup and do not ask config for generator (only fall 
		# back to prompt if not specified as arg)
		self.ensure_setup_latest()
		
		if sys.platform == "darwin":
			config = self.getConfig()
		
			if self.macSdk:
				config.set('hm', 'macSdk', self.macSdk)
			elif config.has_option("hm", "macSdk"):
				self.macSdk = config.get('hm', 'macSdk')
		
			if self.macIdentity:
				config.set('hm', 'macIdentity', self.macIdentity)
			elif config.has_option("hm", "macIdentity"):
				self.macIdentity = config.get('hm', 'macIdentity')
		
			self.write_config(config)
		
			if not self.macSdk:
				raise Exception("Arg missing: --mac-sdk <version>");
				
			if not self.macIdentity:
				raise Exception("Arg missing: --mac-identity <name>");
			
			sdkDir = self.getMacSdkDir()
			if not os.path.exists(sdkDir):
				raise Exception("Mac SDK not found at: " + sdkDir)
			
			os.environ["MACOSX_DEPLOYMENT_TARGET"] = self.macSdk
		
		# default is release
		if target == '':
			print 'Defaulting target to: ' + self.defaultTarget
			target = self.defaultTarget
		
		# allow user to skip core compile
		if self.enableMakeCore:
			self.configureCore(target, extraArgs)
		
		# allow user to skip gui compile
		if self.enableMakeGui:
			self.configureGui(target, extraArgs)
		
		self.setConfRun(target)

	def configureCore(self, target="", extraArgs=""):
		
		# ensure that we have access to cmake
		_cmake_cmd = self.persist_cmake()
	
		# now that we know we've got the latest setup, we can ask the config
		# file for the generator (but again, we only fall back to this if not 
		# specified as arg).
		generator = self.getGenerator()
		
		if generator != self.findGeneratorFromConfig():
			print('Generator changed, running setup.')
			self.setup(target)
	
		cmake_args = ''
		if generator.cmakeName != '':
			cmake_args += ' -G "' + generator.cmakeName + '"'

		# for makefiles always specify a build type (debug, release, etc)
		if generator.cmakeName.find('Unix Makefiles') != -1:
			cmake_args += ' -DCMAKE_BUILD_TYPE=' + target.capitalize()
			
		elif sys.platform == "darwin":
			macSdkMatch = re.match("(\d+)\.(\d+)", self.macSdk)
			if not macSdkMatch:
				raise Exception("unknown osx version: " + self.macSdk)

			sdkDir = self.getMacSdkDir()
			cmake_args += " -DCMAKE_OSX_SYSROOT=" + sdkDir
			cmake_args += " -DCMAKE_OSX_DEPLOYMENT_TARGET=" + self.macSdk
			cmake_args += " -DOSX_TARGET_MAJOR=" + macSdkMatch.group(1)
			cmake_args += " -DOSX_TARGET_MINOR=" + macSdkMatch.group(2)
		
		# if not visual studio, use parent dir
		sourceDir = generator.getSourceDir()

		self.checkGTest()
		self.checkGMock()
		
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

	def configureGui(self, target="", extraArgs=""):
			
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
			
			libs = (
				"-framework ApplicationServices "
				"-framework Security "
				"-framework cocoa")
			
			if major == 10 and minor >= 6:
				libs += " -framework ServiceManagement"
			
			qmake_cmd_string += " \"MACX_LIBS=%s\" " % libs
			
			sdkDir = self.getMacSdkDir()
			shortForm = "macosx" + self.macSdk
			version = str(major) + "." + str(minor)
			
			qmake_cmd_string += " QMAKE_MACOSX_DEPLOYMENT_TARGET=" + version

			(qMajor, qMinor, qRev) = self.getQmakeVersion()
			if qMajor <= 4:
				# 4.6: qmake takes full sdk dir.
				qmake_cmd_string += " QMAKE_MAC_SDK=" + sdkDir
			else:
				# 5.2: now we need to use the .path setting.
				qmake_cmd_string += " QMAKE_MAC_SDK=" + shortForm
				qmake_cmd_string += " QMAKE_MAC_SDK." + shortForm + ".path=" + sdkDir

		print "QMake command: " + qmake_cmd_string
		
		# run qmake from the gui dir
		self.try_chdir(self.gui_dir)
		err = os.system(qmake_cmd_string)
		self.restore_chdir()
		
		if err != 0:
			raise Exception('QMake encountered error: ' + str(err))

	def getQmakeVersion(self):
		version = commands.getoutput("qmake --version")
		result = re.search('(\d+)\.(\d+)\.(\d)', version)
		
 		if not result:
			raise Exception("Could not get qmake version.")
 
 		major = int(result.group(1))
 		minor = int(result.group(2))
 		rev = int(result.group(3))
 		
 		return (major, minor, rev)

	def getMacSdkDir(self):
		sdkName = "macosx" + self.macSdk

		# Ideally we'll use xcrun (which is influenced by $DEVELOPER_DIR), then try a couple
		# fallbacks to known paths if xcrun is not available
		status, sdkPath = commands.getstatusoutput("xcrun --show-sdk-path --sdk " + sdkName)
		if status == 0 and sdkPath:
			return sdkPath

		developerDir = os.getenv("DEVELOPER_DIR")
		if not developerDir:
			developerDir = "/Applications/Xcode.app/Contents/Developer"

		sdkDirName = sdkName.replace("macosx", "MacOSX")
		sdkPath = developerDir + "/Platforms/MacOSX.platform/Developer/SDKs/" + sdkDirName + ".sdk"
		if os.path.exists(sdkPath):
			return sdkPath

		return "/Developer/SDKs/" + sdkDirName + ".sdk"
	
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
		
		self.loadConfig()

		# allow user to skip core compile
		if self.enableMakeCore:
			self.makeCore(targets)

		# allow user to skip gui compile
		if self.enableMakeGui:
			self.makeGui(targets)
	
	def loadConfig(self):
		config = self.getConfig()
		
		if config.has_option("hm", "macSdk"):
			self.macSdk = config.get("hm", "macSdk")
		
		if config.has_option("hm", "macIdentity"):
			self.macIdentity = config.get("hm", "macIdentity")
	
	def makeCore(self, targets):
	
		generator = self.getGeneratorFromConfig().cmakeName
		
		if self.macSdk:
			os.environ["MACOSX_DEPLOYMENT_TARGET"] = self.macSdk

		if generator.find('Unix Makefiles') != -1:
			for target in targets:
				self.runBuildCommand(self.make_cmd, target)
		else:
			for target in targets:
				if generator.startswith('Visual Studio'):
					self.run_vcbuild(generator, target, self.sln_filepath())
				elif generator == 'Xcode':
					cmd = self.xcodebuild_cmd + ' -configuration ' + target.capitalize()
					self.runBuildCommand(cmd, target)
				else:
					raise Exception('Build command not supported with generator: ' + generator)
	
	def makeGui(self, targets, args=""):
		for target in targets:
		
			if sys.platform == 'win32':

				gui_make_cmd = self.w32_make_cmd + ' ' + target + args
				print 'Make GUI command: ' + gui_make_cmd

				self.try_chdir(self.gui_dir)
				err = os.system(gui_make_cmd)
				self.restore_chdir()
				
				if err != 0:
					raise Exception(gui_make_cmd + ' failed with error: ' + str(err))

			elif sys.platform in ['linux2', 'sunos5', 'freebsd7', 'darwin']:

				gui_make_cmd = self.make_cmd + " -w" + args
				print 'Make GUI command: ' + gui_make_cmd

				# start with a clean app bundle
				targetDir = self.getGenerator().getBinDir(target)
				bundleTargetDir = targetDir + '/Synergy.app'
				if os.path.exists(bundleTargetDir):
					shutil.rmtree(bundleTargetDir)

				binDir = self.getGenerator().binDir
				bundleTempDir = binDir + '/Synergy.app'
				if os.path.exists(bundleTempDir):
					shutil.rmtree(bundleTempDir)

				self.try_chdir(self.gui_dir)
				err = os.system(gui_make_cmd)
				self.restore_chdir()

				if err != 0:
					raise Exception(gui_make_cmd + ' failed with error: ' + str(err))

				if sys.platform == 'darwin' and not "clean" in args:
					self.macPostGuiMake(target)

					self.fixQtFrameworksLayout(target)
			else:
				raise Exception('Unsupported platform: ' + sys.platform)

	def macPostGuiMake(self, target):
		bundle = 'Synergy.app'
		binDir = self.getGenerator().binDir
		targetDir = self.getGenerator().getBinDir(target)
		bundleTempDir = binDir + '/' + bundle
		bundleTargetDir = targetDir + '/' + bundle

		if os.path.exists(bundleTempDir):
			shutil.move(bundleTempDir, bundleTargetDir)

		if self.enableMakeCore:
			# copy core binaries into the bundle, since the gui
			# now looks for the binaries in the current app dir.
			
			bundleBinDir = bundleTargetDir + "/Contents/MacOS/"
			shutil.copy(targetDir + "/synergyc", bundleBinDir)
			shutil.copy(targetDir + "/synergys", bundleBinDir)
			shutil.copy(targetDir + "/syntool", bundleBinDir)

		self.loadConfig()
		if not self.macIdentity:
			raise Exception("run config with --mac-identity")

		if self.enableMakeGui:
			# use qt to copy libs to bundle so no dependencies are needed. do not create a
			# dmg at this point, since we need to sign it first, and then create our own
			# after signing (so that qt does not affect the signed app bundle).
			bin = "macdeployqt Synergy.app -verbose=2"
			self.try_chdir(targetDir)
			err = os.system(bin)
			self.restore_chdir()
			print bundleTargetDir
			if err != 0:
				raise Exception(bin + " failed with error: " + str(err))

			(qMajor, qMinor, qRev) = self.getQmakeVersion()
			if qMajor <= 4:
				frameworkRootDir = "/Library/Frameworks"
			else:
				# TODO: auto-detect, qt can now be installed anywhere.
				frameworkRootDir = "/Developer/Qt5.2.1/5.2.1/clang_64/lib"

			target = bundleTargetDir + "/Contents/Frameworks"

			# copy the missing Info.plist files for the frameworks.
			for root, dirs, files in os.walk(target):
				for dir in dirs:
					if dir.startswith("Qt"):
						shutil.copy(
							frameworkRootDir + "/" + dir + "/Contents/Info.plist",
							target + "/" + dir + "/Resources/")

	def symlink(self, source, target):
		if not os.path.exists(target):
			os.symlink(source, target)

	def move(self, source, target):
		if os.path.exists(source):
			shutil.move(source, target)

	def fixQtFrameworksLayout(self, target):
		# reorganize Qt frameworks layout on Mac 10.9.5 or later
		# http://goo.gl/BFnQ8l
		# QtCore example:
		# 	QtCore.framework/
		# 		QtCore    -> Versions/Current/QtCore
		# 		Resources -> Versions/Current/Resources
		# 		Versions/
		# 			Current -> 5
		# 			5/
		# 				QtCore
		# 				Resources/
		# 					Info.plist
		targetDir = self.getGenerator().getBinDir(target)

		target = targetDir + "/Synergy.app/Contents/Frameworks"
		(major, minor) = self.getMacVersion()
		if major == 10:
			if minor >= 9:
				for root, dirs, files in os.walk(target):
					for dir in dirs:
						if dir.startswith("Qt"):
							self.try_chdir(target + "/" + dir +"/Versions")
							self.symlink("5", "Current")
							self.move("../Resources", "5")
							self.restore_chdir()

							self.try_chdir(target + "/" + dir)
							dot = dir.find('.')
							frameworkName = dir[:dot]
							self.symlink("Versions/Current/" + frameworkName, frameworkName)
							self.symlink("Versions/Current/Resources", "Resources")
							self.restore_chdir()

	def signmac(self):
		self.loadConfig()
		if not self.macIdentity:
			raise Exception("run config with --mac-identity")

		self.try_chdir("bin/Release/")
		err = os.system(
			'codesign --deep -fs "' + self.macIdentity + '" Synergy.app')
		self.restore_chdir()

		if err != 0:
			raise Exception("codesign failed with error: " + str(err))
	
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
			self.signFile(pfx, pwd, 'bin/Release', 'syntool.exe')
			self.signFile(pfx, pwd, 'bin/Release', 'synwinhk.dll')
	
	def signFile(self, pfx, pwd, dir, file):
		self.try_chdir(dir)
		err = os.system(
			'signtool sign'
			' /f ' + pfx +
			' /p ' + pwd +
			' /t http://timestamp.verisign.com/scripts/timstamp.dll ' +
			file)
		self.restore_chdir()

		if err != 0:
			raise Exception("signtool failed with error: " + str(err))
	
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
		
		# allow user to skip core clean
		if self.enableMakeCore:
			self.cleanCore(targets)

		# allow user to skip qui clean
		if self.enableMakeGui:
			self.cleanGui(targets)
	
	def cleanCore(self, targets):
		generator = self.getGeneratorFromConfig().cmakeName

		if generator.startswith('Visual Studio'):
			# special case for version 10, use new /target:clean
			if generator.startswith('Visual Studio 10'):
				for target in targets:
					self.run_vcbuild(generator, target, self.sln_filepath(), '/target:clean')
				
			# any other version of visual studio, use /clean
			elif generator.startswith('Visual Studio'):
				for target in targets:
					self.run_vcbuild(generator, target, self.sln_filepath(), '/clean')

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
	
	def cleanGui(self, targets):
		self.makeGui(targets, " clean")
		
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
		return self.getGitRevision()

	def getGitRevision(self):
		if sys.version_info < (2, 4):
			raise Exception("Python 2.4 or greater required.")
		
		p = subprocess.Popen(
			["git", "log", "--pretty=format:%h", "-n", "1"],
			stdout=subprocess.PIPE, stderr=subprocess.PIPE)

		stdout, stderr = p.communicate()

		if p.returncode != 0:
			raise Exception('Could not get revision, git error: ' + str(p.returncode))
	
		return stdout.strip()

	def getGitBranchName(self):
		if sys.version_info < (2, 4):
			raise Exception("Python 2.4 or greater required.")
		
		p = subprocess.Popen(
			["git", "rev-parse", "--abbrev-ref", "HEAD"],
			stdout=subprocess.PIPE, stderr=subprocess.PIPE)

		stdout, stderr = p.communicate()

		if p.returncode != 0:
			raise Exception('Could not get branch name, git error: ' + str(p.returncode))
	
		return stdout.strip()

	def find_revision_svn(self):
		if sys.version_info < (2, 4):
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
		self.enableMakeGui = False
		
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
			self.dist_usage()
			return

		moveExt = ''

		if type == 'src':
			self.distSrc()
			
		elif type == 'rpm':
			if sys.platform == 'linux2':
                                self.distRpm()
			else:
				package_unsupported = True
			
		elif type == 'deb':
			if sys.platform == 'linux2':
				self.distDeb()
			else:
				package_unsupported = True
			
		elif type == 'win':
			if sys.platform == 'win32':
				#self.distNsis(vcRedistDir, qtDir)
				self.distWix()
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
		
        def distRpm(self):
                rpmDir = self.getGenerator().buildDir + '/rpm'
                if os.path.exists(rpmDir):
			shutil.rmtree(rpmDir)
		
		os.makedirs(rpmDir)

		templateFile = open(self.cmake_dir + '/synergy.spec.in')
		template = templateFile.read()

		template = template.replace('${in:version}', self.getVersionFromCmake())	
                
		specPath = rpmDir + '/synergy.spec'

		specFile = open(specPath, 'w')
		specFile.write(template)
		specFile.close()

                version = self.getVersionFromCmake()
                target = '../../bin/synergy-%s-%s.rpm' % (
                        version, self.getLinuxPlatform())
                

		try:
			self.try_chdir(rpmDir)
                        cmd = 'rpmbuild -bb --define "_topdir `pwd`" synergy.spec'
                        print "Command: " + cmd
			err = os.system(cmd)
			if err != 0:
				raise Exception('rpmbuild failed: ' + str(err))

			self.unixMove('RPMS/*/*.rpm', target)

                        cmd = 'rpmlint ' + target
                        print "Command: " + cmd
			err = os.system(cmd)
			if err != 0:
				raise Exception('rpmlint failed: ' + str(err))
		finally:
			self.restore_chdir()

        def distDeb(self):
		buildDir = self.getGenerator().buildDir
		binDir = self.getGenerator().binDir
		resDir = self.cmake_dir

                version = self.getVersionFromCmake()
                package = '%s-%s-%s' % (
                        self.project, version, self.getLinuxPlatform())

                debDir = '%s/deb' % buildDir
                if os.path.exists(debDir):
			shutil.rmtree(debDir)

		metaDir = '%s/%s/DEBIAN' % (debDir, package)		
		os.makedirs(metaDir)

		templateFile = open(resDir + '/deb/control.in')
		template = templateFile.read()

		template = template.replace('${in:version}',
			self.getVersionFromCmake())

		template = template.replace('${in:arch}',
			self.getDebianArch())

		controlPath = '%s/control' % metaDir

		controlFile = open(controlPath, 'w')
		controlFile.write(template)
		controlFile.close()

		targetBin = '%s/%s/usr/bin' % (debDir, package)
		targetShare = '%s/%s/usr/share' % (debDir, package)
		targetApplications = "%s/applications" % targetShare
		targetIcons = "%s/icons" % targetShare
		targetDocs = "%s/doc/%s" % (targetShare, self.project)

		os.makedirs(targetBin)
		os.makedirs(targetApplications)
		os.makedirs(targetIcons)
		os.makedirs(targetDocs)
		
		for root, dirs, files in os.walk(debDir):
			for d in dirs:
				os.chmod(os.path.join(root, d), 0o0755)

		binFiles = ['synergy', 'synergyc', 'synergys', 'synergyd', 'syntool']
		for f in binFiles:
			shutil.copy("%s/%s" % (binDir, f), targetBin)
			target = "%s/%s" % (targetBin, f)
			os.chmod(target, 0o0755)
			err = os.system("strip " + target)
			if err != 0:
				raise Exception('strip failed: ' + str(err))

		shutil.copy("%s/synergy.desktop" % resDir, targetApplications)
		shutil.copy("%s/synergy.ico" % resDir, targetIcons)

		docTarget = "%s/doc/%s" % (targetShare, self.project)

		copyrightPath = "%s/deb/copyright" % resDir
		shutil.copy(copyrightPath, docTarget)

		shutil.copy("%s/deb/changelog" % resDir, docTarget)
		os.system("gzip -9 %s/changelog" % docTarget)
		if err != 0:
			raise Exception('gzip failed: ' + str(err))

		for root, dirs, files in os.walk(targetShare):
			for f in files:
				os.chmod(os.path.join(root, f), 0o0644)
		
		target = '../../bin/%s.deb' % package

		try:
			self.try_chdir(debDir)

			# TODO: consider dpkg-buildpackage (higher level tool)
                        cmd = 'fakeroot dpkg-deb --build %s' % package
                        print "Command: " + cmd
			err = os.system(cmd)
			if err != 0:
				raise Exception('dpkg-deb failed: ' + str(err))

                        cmd = 'lintian %s.deb' % package
                        print "Command: " + cmd
			err = os.system(cmd)
			if err != 0:
				raise Exception('lintian failed: ' + str(err))

			self.unixMove('*.deb', target)
		finally:
			self.restore_chdir()

	def distSrc(self):
		version = self.getVersionFromCmake()
		name = (self.project + '-' + version + '-Source')
		exportPath = self.getGenerator().buildDir + '/' + name

		if os.path.exists(exportPath):
			print "Removing existing export..."
			shutil.rmtree(exportPath)

		os.mkdir(exportPath)

		cmd = "git archive %s | tar -x -C %s" % (
			self.getGitBranchName(), exportPath)
		
		print 'Exporting repository to: ' + exportPath
		err = os.system(cmd)
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
		self.loadConfig()
		binDir = self.getGenerator().getBinDir('Release')
		name = "Synergy"
		dist = binDir + "/" + name
		
		# ensure dist dir is clean
		if os.path.exists(dist):
			shutil.rmtree(dist)
		
		os.makedirs(dist)
		shutil.move(binDir + "/" + name + ".app", dist + "/" + name + ".app")
		
		self.try_chdir(dist)
		err = os.system("ln -s /Applications")
		self.restore_chdir()
		
		fileName = "%s-%s-%s.dmg" % (
			self.project, 
			self.getVersionFromCmake(),
			self.getMacPackageName())
		
		cmd = "hdiutil create " + fileName + " -srcfolder ./" + name + "/ -ov"
		
		self.try_chdir(binDir)
		err = os.system(cmd)
		self.restore_chdir()

	def distWix(self):
		generator = self.getGeneratorFromConfig().cmakeName
		
		arch = 'x86'
		if generator.endswith('Win64'):
			arch = 'x64'
		
		version = self.getVersionFromCmake()
		args = "/p:DefineConstants=\"Version=%s\"" % version
		
		self.run_vcbuild(
			generator, 'release', 'synergy.sln', args,
			'src/setup/win32/', 'x86')
		
		filename = "%s-%s-Windows-%s.msi" % (
			self.project, 
			version,
			arch)
			
		old = "bin/Release/synergy.msi"
		new = "bin/%s" % (filename)
		
		try:
			os.remove(new)
		except OSError:
			pass
		
		os.rename(old, new)
		
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
		
		self.loadConfig()
		src = self.dist_name(type)
		dest = self.dist_name_rev(type)
		print 'Uploading %s to FTP server %s...' % (dest, ftp.host)

		binDir = self.getGenerator().getBinDir('Release')
		ftp.run(binDir + '/' + src, dest) 
		print 'Done'
	
	def getDebianArch(self):
		if os.uname()[4][:3] == 'arm':
			return 'armhf'

                # os_bits should be loaded with '32bit' or '64bit'
                import platform
                (os_bits, other) = platform.architecture()
		
                # get platform based on current platform
                if os_bits == '32bit':
                        return 'i386'
                elif os_bits == '64bit':
                        return 'amd64'
                else:
                        raise Exception("unknown os bits: " + os_bits)

        def getLinuxPlatform(self):
		if os.uname()[4][:3] == 'arm':
			return 'Linux-armv6l'

                # os_bits should be loaded with '32bit' or '64bit'
                import platform
                (os_bits, other) = platform.architecture()
		
                # get platform based on current platform
                if os_bits == '32bit':
                        return 'Linux-i686'
                elif os_bits == '64bit':
                        return 'Linux-x86_64'
                else:
                        raise Exception("unknown os bits: " + os_bits)

	def dist_name(self, type):
		ext = None
		platform = None
		
		if type == 'src':
			ext = 'tar.gz'
			platform = 'Source'
			
		elif type == 'rpm' or type == 'deb':
		
			ext = type
                        platform = self.getLinuxPlatform()
			
		elif type == 'win':
			
			# get platform based on last generator used
			ext = 'msi'
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
		
		target = ''
		if type == 'mac':
			target = 'Release'

		for filename in os.listdir(self.getBinDir(target)):
			if re.search(pattern, filename):
				return filename
		
		# still here? package probably not created yet.
		raise Exception('Could not find package name with pattern: ' + pattern)
	
	def dist_name_rev(self, type):
		# find the version number (we're puting the rev in after this)
		pattern = '(\d+\.\d+\.\d+)'
		replace = "%s-%s" % (
			self.getGitBranchName(), self.getGitRevision())
		return re.sub(pattern, replace, self.dist_name(type))

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

	def run_vcbuild(self, generator, mode, solution, args='', dir='', config32='Win32'):
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
			config_platform = config32
		
		if mode == 'release':
			config = 'Release'
		else:
			config = 'Debug'
				
		if generator.startswith('Visual Studio 10'):
			cmd = ('@echo off\n'
				'call "%s" %s \n'
				'cd "%s"\n'
				'msbuild /nologo %s /p:Configuration="%s" /p:Platform="%s" "%s"'
				) % (self.get_vcvarsall(generator), vcvars_platform, dir, args, config, config_platform, solution)
		else:
			config = config + '|' + config_platform
			cmd = ('@echo off\n'
				'call "%s" %s \n'
				'cd "%s"\n'
				'vcbuild /nologo %s "%s" "%s"'
				) % (self.get_vcvarsall(generator), vcvars_platform, dir, args, solution, config)
		
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
		if not self.macSdk:
			raise Exception("Mac OS X SDK not set.")
		
		result = re.search('(\d+)\.(\d+)', self.macSdk)
		if not result:
			print versions
			raise Exception("Could not find Mac OS X version.")

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
		
		# qt 4.3 generates ui_ files.
		for filename in glob.glob("src/gui/ui_*"):
		  os.remove(filename)

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
			elif o == '--skip-gui':
				self.ic.enableMakeGui = False
			elif o == '--skip-core':
				self.ic.enableMakeCore = False
			elif o in ('-d', '--debug'):
				self.build_targets += ['debug',]
			elif o in ('-r', '--release'):
				self.build_targets += ['release',]
			elif o == '--vcredist-dir':
				self.vcRedistDir = a
			elif o == '--qt-dir':
				self.qtDir = a
			elif o == '--mac-sdk':
				self.ic.macSdk = a
			elif o == '--mac-identity':
				self.ic.macIdentity = a
	
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
		self.ic.doxygen()
	
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
		self.ic.signmac()

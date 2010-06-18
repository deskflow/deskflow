# TODO: split this file up, it's too long!

import sys, os, ConfigParser, subprocess, shutil, re

class InternalCommands:

	project = 'synergy-plus'
	setup_version = 3
	website_url = 'http://code.google.com/p/synergy-plus'

	this_cmd = 'hm'
	cmake_cmd = 'cmake'

	make_cmd = 'make'
	xcodebuild_cmd = 'xcodebuild'

	source_dir = '..' # Source, relative to build.
	cmake_dir = 'cmake'
	bin_dir = 'bin'

	sln_filename = '%s.sln' % project
	xcodeproj_filename = '%s.xcodeproj' % project
	config_filename = '%s.cfg' % this_cmd

	# try_chdir(...) and restore_chdir() will use this
	prevdir = ''
	
	# by default, no index specified as arg
	generator_id = None
	
	# by default, prompt user for input
	no_prompts = False

	win32_generators = {
		'1' : 'Visual Studio 10',
		'2' : 'Visual Studio 10 Win64',
		'3' : 'Visual Studio 9 2008',
		'4' : 'Visual Studio 9 2008 Win64',
		'5' : 'Visual Studio 8 2005',
		'6' : 'Visual Studio 8 2005 Win64',
		'10' : 'CodeBlocks - MinGW Makefiles',
		'11' : 'CodeBlocks - Unix Makefiles',
		'12': 'Eclipse CDT4 - MinGW Makefiles',
		'13': 'Eclipse CDT4 - NMake Makefiles',
		'14': 'Eclipse CDT4 - Unix Makefiles',
		'15': 'MinGW Makefiles',
		'16': 'NMake Makefiles',
		'17': 'Unix Makefiles',
		'18': 'Borland Makefiles',
		'19': 'MSYS Makefiles',
		'20': 'Watcom WMake',
	}

	unix_generators = {
		'1' : 'Unix Makefiles',
		'2' : 'CodeBlocks - Unix Makefiles',
		'3' : 'Eclipse CDT4 - Unix Makefiles',
		'4' : 'KDevelop3',
		'5' : 'KDevelop3 - Unix Makefiles',
	}

	darwin_generators = {
		'1' : 'Xcode',
		'2' : 'Unix Makefiles',
		'3' : 'CodeBlocks - Unix Makefiles',
		'4' : 'Eclipse CDT4 - Unix Makefiles',
		'5' : 'KDevelop3',
		'6' : 'KDevelop3 - Unix Makefiles',
	}

	def config_filepath(self):
		return '%s/%s' % (self.bin_dir, self.config_filename)

	def sln_filepath(self):
		return '%s\%s' % (self.bin_dir, self.sln_filename)

	def xcodeproj_filepath(self):
		return '%s/%s' % (self.bin_dir, self.xcodeproj_filename)
		
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
			'  reformat    Reformat .cpp and .h files using AStyle\n'
			'  usage       Shows the help screen\n'
			'\n'
			'Example: %s build -g 3'
			) % (app, app)
	
	def configure(self):
		self.configure_internal()
		
		print ('Configure complete!\n\n'
			'Open project now: %s open\n'
			'Command line build: %s build'
			) % (self.this_cmd, self.this_cmd)

	def configure_internal(self):
		
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
			cmake_args = '%s -G "%s"' % (self.source_dir, generator)
		else:
			cmake_args = self.source_dir
		
		cmake_cmd_string = '%s %s' % (_cmake_cmd, cmake_args)

		print "Configuring with CMake (%s)..." % cmake_cmd_string

		# Run from build dir so we have an out-of-source build.
		self.try_chdir(self.bin_dir)
		err = os.system(cmake_cmd_string)
		self.restore_chdir()

		if err != 0:
			raise Exception('CMake encountered error: ' + str(err))
		
		self.set_conf_run()

	def persist_cmake(self):
		if sys.platform == 'win32':
		
			version = '>= 2.8.0'
			found_cmd = ''
			for test_cmd in (self.cmake_cmd, r'tool\cmake\bin\%s' % self.cmake_cmd):
				print 'Testing for CMake version %s by running `%s`...' % (version, test_cmd)
				p = subprocess.Popen([test_cmd, '--version'], stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
				stdout, stderr = p.communicate()
				m = re.search('cmake version (2\.8\.\d+)', stdout)
				if p.returncode == 0 and m:
					# found one that works, hurrah!
					print 'Found valid CMake version %s' % m.group(1)
					found_cmd = test_cmd
					# HACK: gotta go out so just hacking this for now
					if found_cmd == r'tool\cmake\bin\%s' % self.cmake_cmd:
						found_cmd = r'..\tool\cmake\bin\%s' % self.cmake_cmd
					break
			
			if not found_cmd:
				
				found_cmake = False
				
				# if prompting allowed
				if not self.no_prompts:
					msg = 'CMake 2.8.0 not installed. Auto download now? [Y/n]'
					print msg,
					yn = raw_input()
					
					# if response was anyting but no
					if yn not in ['n', 'N']:
						if not os.path.exists('tool'):
							os.mkdir('tool')
						
						err = os.system(r'svn checkout https://synergy-plus.googlecode.com/svn/tools/win/cmake tool\cmake')
						if err != 0:
							raise Exception('Unable to get cmake from repository with error code code: ' + str(err))
						
						found_cmd = r'..\tool\cmake\bin\%s' % self.cmake_cmd
						found_cmake = True
				
				# if cmake was not found
				if not found_cmake:
					raise Exception('Cannot continue without CMake, exiting.')
			
			return found_cmd
		else:
			return self.cmake_cmd

	def build(self, mode = None):

		self.ensure_setup_latest()

		if not self.has_conf_run():
			self.configure_internal()
		
		generator = self.get_generator_from_config()

		if generator == "Unix Makefiles":

			print 'Building with GNU Make...'
			self.try_chdir(self.bin_dir)
			err = os.system(self.make_cmd)
			self.restore_chdir()
			
			if err != 0:
				raise Exception('GNU Make failed: ' + str(err))

		elif generator.startswith('Visual Studio'):
			
			self.run_vcbuild(generator, mode)

		elif generator == 'Xcode':        

			print 'Building with Xcode...'
			self.try_chdir(self.bin_dir)
			err = os.system(self.xcodebuild_cmd)
			self.restore_chdir()

			if err != 0:
				raise Exception('Xcode failed:', err)
			
		else:
			raise Exception('Not supported with generator: ' + generator)

	def clean(self, mode = None):
		
		generator = self.get_generator_from_config()

		if generator == "Unix Makefiles":

			print 'Cleaning with GNU Make...'
			self.try_chdir(self.bin_dir)
			err = os.system(self.make_cmd + ' clean')
			self.restore_chdir()

			if err != 0:
				raise Exception('GNU Make failed: ' + str(err))

		# special case for version 10, use new /target:clean
		elif generator.startswith('Visual Studio 10'):

			self.run_vcbuild(generator, mode, '/target:clean')

		# any other version of visual studio, use /clean
		elif generator.startswith('Visual Studio'):

			self.run_vcbuild(generator, mode, '/clean')

		elif generator == 'Xcode':        

			print 'Cleaning with Xcode...'
			self.try_chdir(self.bin_dir)
			err = os.system(xcodebuild_cmd + ' clean')
			self.restore_chdir()

			if err != 0:
				raise Exception('Xcode failed:', err)
			
		else:
			raise Exception('Not supported with generator: ' + generator)

	def open(self):
		generator = self.get_generator_from_config()
		if generator.startswith('Visual Studio'):
			print 'Opening with %s...' % generator
			self.open_internal(self.sln_filepath())
			
		elif generator.startswith('Xcode'):
			print 'Opening with %s...' % generator
			self.open_internal(xcodeproj_filepath(), 'open')
			
		else:
			raise Exception('Not supported with generator: ' + generator)
		
	def update(self):
		print "Running Subversion update..."
		err = os.system('svn update')
		if err != 0:
			raise Exception('Could not update from repository with error code code: ' + str(err))
		
	def revision(self):
		# While this doesn't print out the revision specifically, it will do.
		err = os.system('svn info')
		if err != 0:
			raise Exception('Could not get revision info with error code code: ' + str(err))

	def kill(self):
		if sys.platform == 'win32':
			return os.system('taskkill /F /FI "IMAGENAME eq synergy*"')
		else:
			raise Exception('Not implemented for platform: ' + sys.platform)
				
	def package(self, type):

		# Package is supported by default.
		package_unsupported = False

		if type == None:
			self.package_usage()
		elif type == 'src':
			if sys.platform in ['linux2', 'darwin']:
				self.package_run('make package_source')
			else:
				package_unsupported = True
		elif type == 'rpm':
			if sys.platform == 'linux2':
				self.package_run('cpack -G RPM')
			else:
				package_unsupported = True
		elif type == 'deb':
			if sys.platform == 'linux2':
				self.package_run('cpack -G DEB')
			else:
				package_unsupported = True
		elif type == 'win':
			if sys.platform == 'win32':
				self.package_run('cpack -G NSIS')
			else:
				package_unsupported = True
		elif type == 'mac':
			if sys.platform == 'darwin':
				self.package_run('cpack -G PackageMaker')
			else:
				package_unsupported = True
		else:
			print 'Not yet implemented: package %s' % type

		if package_unsupported:
			raise Exception(
				("Package type, '%s' is not supported for platform, '%s'") 
				% (type, sys.platform))
	
	def package_run(self, command):
		self.try_chdir(self.bin_dir)
		err = os.system(command)
		self.restore_chdir()
		if err != 0:
			raise Exception('Package failed: ' + str(err))

	def package_usage(self):
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
		print ('Help Me script, from the Synergy+ project.\n'
			'%s\n'
			'\n'
			'For help, run: %s help') % (self.website_url, self.this_cmd)

	def try_chdir(self, dir):

		# Ensure temp build dir exists.
		if not os.path.exists(dir):
			os.mkdir(dir)

		global prevdir    
		prevdir = os.path.abspath(os.curdir)

		# It will exist by this point, so it's safe to chdir.
		os.chdir(dir)

	def restore_chdir(self):
		global prevdir
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

	def setup(self):
		print "Running setup..."

		# always either get generator from args, or prompt user when 
		# running setup
		generator = self.get_generator_from_prompt()

		# Create build dir, since config file resides there.
		if not os.path.exists(self.bin_dir):
			os.mkdir(self.bin_dir)

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

		cmakecache_filename = '%s/CMakeCache.txt' % self.bin_dir
		if os.path.exists(cmakecache_filename):
			print "Removing %s, since generator changed." % cmakecache_filename
			os.remove(cmakecache_filename)

		print "\nSetup complete."

	def write_config(self, config):
		configfile = open(self.config_filepath(), 'wb')
		config.write(configfile)

	def get_generator_from_config(self):
		if self.generator_id:
			generators = self.get_generators()
			return generators[self.generator_id]
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

	def has_conf_run(self):
		if self.min_setup_version(2):
			config = ConfigParser.RawConfigParser()
			config.read(self.config_filepath())
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
		
		generators = self.get_generators()
		
		# if user has specified a generator as an argument
		if self.generator_id:
			return generators[self.generator_id]
		
		# if we can accept user input
		elif not self.no_prompts:
			generator_options = ''
			generators_sorted = sorted(generators.iteritems(), key=lambda t: int(t[0]))
			
			for id, generator in generators_sorted:
				generator_options += '\n  ' + id + ': ' + generator

			print ('\nChoose a CMake generator:%s'
				) % generator_options

			return self.setup_generator_prompt(generators)
			
		else:
			raise Exception('No generator specified, and cannot prompt user.')

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
		temp_bat = self.bin_dir + r'\vcbuild.bat'
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

# the command handler should be called only from hm.py (i.e. directly 
# from the command prompt). the purpose of this class is so that we 
# don't need to do argument handling all over the place in the internal
# commands class.
class CommandHandler:
	ic = InternalCommands()

	def __init__(self, argv, opts, args):
		
		self.opts = opts
		self.args = args
		
		for o, a in self.opts:
			if o == '--no-prompts':
				self.ic.no_prompts = True
			elif o in ('-g', '--generator'):
				self.ic.generator_id = a

	def get_build_mode(self):
		mode = None
		for o, a in self.opts:
			if o == ('-d', '--debug'):
				mode = 'debug'
			elif o == ('-r', '--release'):
				mode = 'release'
		return mode
	
	def about(self):
		self.ic.about()
	
	def setup(self):
		self.ic.setup()
	
	def configure(self):
		self.ic.configure()
	
	def build(self):
		self.ic.build(self.get_build_mode())
	
	def clean(self):
		self.ic.clean(self.get_build_mode())
	
	def update(self):
		self.ic.update()
	
	def install(self):
		print 'Not yet implemented: install'
	
	def package(self):
		self.ic.package()
	
	def destroy(self):
		self.ic.destroy()
	
	def package(self):
		type = None
		if len(self.args) > 0:
			type = self.args[0]
		self.ic.package(type)
	
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

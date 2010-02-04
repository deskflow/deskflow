#! /usr/bin/env python

# hm.py: 'Help Me', is a simple wrapper for `cmake` and `hg`.
# 
# This script was created for the Synergy+ project.
# http://code.google.com/p/synergy-plus
#
# The idea behind this is to simplify the usage of CMake,
# while still making cmake independant of this script. In
# other words, you don't need to use this script.
#
# If you don't wish to run this script, simply run:
#   cmake .
#   make
# This will create an in-source UNIX Makefile.

import sys, os, ConfigParser, subprocess, shutil

if sys.platform == 'win32':
	import _winreg

project = 'synergy-plus'
setup_version = 3
website_url = 'http://code.google.com/p/synergy-plus'

this_cmd = 'hm'
cmake_cmd = 'cmake'
if sys.platform == 'win32':
	cmake_cmd = os.getcwd() + '\\tool\\win\\cmake\\bin\\'+ cmake_cmd

make_cmd = 'make'
xcodebuild_cmd = 'xcodebuild'

source_dir = '..' # Source, relative to build.
cmake_dir = 'cmake'
bin_dir = 'bin'
build_dir = 'build' # Obsolete

# Valid commands.
commands = [
	'about',
	'setup',
	'configure',
	'build',
	'clean',
	'update',
	'install',
	'package',
	'dist',
	'open',
	'destroy',
	'kill',
	'usage',
	'revision',
	'help',
	'hammer',
	'reformat',
	'--help',
	'-h',
	'/?'
]

win32_generators = {
	'1' : 'Visual Studio 10',
	'2' : 'Visual Studio 10 Win64',
	'3' : 'Visual Studio 9 2008',
	'4' : 'Visual Studio 9 2008 Win64',
	'5' : 'Visual Studio 8 2005',
	'6' : 'Visual Studio 8 2005 Win64',
	'7' : 'Visual Studio 7',
	'8' : 'Visual Studio 7 .NET 2003',
	'9' : 'Visual Studio 6',
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

sln_filename = '%s.sln' % project
xcodeproj_filename = '%s.xcodeproj' % project
config_filename = '%s.cfg' % this_cmd

def config_filepath():
	return '%s/%s' % (bin_dir, config_filename)

def sln_filepath():
	return '%s\%s' % (bin_dir, sln_filename)

def xcodeproj_filepath():
	return '%s/%s' % (bin_dir, xcodeproj_filename)

# try_chdir(...) and restore_chdir() will use this
prevdir = ''

def usage():
	cmd = sys.argv[0]
	print ('Usage: %s [command]\n'
		'\n'
		'Replace [command] with one of:\n'
		'  about       Show information about this script\n'
		'  setup       Runs the initial setup for this script\n'
		'  configure   Runs cmake (generates project files)\n'
		'  open        Attempts to open the generated project file\n'
		'  build       Builds using the platform build chain\n'
		'  clean       Cleans using the platform build chain\n'
		'  destroy     Destroy all temporary files (bin and build)\n'
		'  kill        Kills all synergy processes (run as admin)"\n'
		'  update      Updates the source code from repository\n'
		'  revision    Display the current source code revision\n'
		'  package     Create a distribution package (e.g. tar.gz)\n'
		'  install     Installs the program\n'
		'  hammer      Golden hammer (config, build, package)\n'
		'  reformat    Reformat .cpp and .h files using AStyle\n'
		'  usage       Shows the help screen\n'
		'\n'
		'Alias commands:\n'
		'  conf        configure\n'
		'  up          update\n'
		'  dist        package\n'
		'  rev         revision\n'
		'\n'
		'Example: %s configure'
		) % (this_cmd, this_cmd)

def configure(generator = None):
	
	err = configure_internal(generator)

	if err == 0:
		print ('Configure complete!\n\n'
			'Open project now: %s open\n'
			'Command line build: %s build'
			) % (this_cmd, this_cmd)
		return True
	else:
		return False

def configure_internal(generator = None):

	ensure_setup_latest(generator)
	
	generator = get_generator()
	if generator != '':
		cmake_args = '%s -G "%s"' % (source_dir, generator)
	else:
		cmake_args = source_dir
	
	cmake_cmd_string = '%s %s' % (cmake_cmd, cmake_args)

	print "Configuring with CMake (%s)..." % cmake_cmd_string

	# Run from build dir so we have an out-of-source build.
	try_chdir(bin_dir)
	err = os.system(cmake_cmd_string)
	restore_chdir()

	if err != 0:
		print 'CMake encountered error:', err
	else:
		set_conf_run()

	return err;

def build(mode):

	ensure_setup_latest()

	if not has_conf_run():
		if configure_internal() != 0:
			return False
	
	generator = get_generator()

	if generator == "Unix Makefiles":

		print 'Building with GNU Make...'
		try_chdir(bin_dir)
		err = os.system(make_cmd)
		restore_chdir()

		if err == 0:
			return True
		else:
			print 'GNU Make failed:', err
			return False

	elif generator.startswith('Visual Studio 10') or generator.startswith('Visual Studio 8') or generator.startswith('Visual Studio 9'):
		
		ret = run_vcbuild(generator, mode)
		
		if ret == 0:
			return True
		else:
			print 'VCBuild failed:', ret
			return False

	elif generator == 'Xcode':        

		print 'Building with Xcode...'
		try_chdir(bin_dir)
		err = os.system(xcodebuild_cmd)
		restore_chdir()

		if err == 0:
			return True
		else:
			print 'Xcode failed:', err
			return False
		
	else:
		print 'Not supported with generator:',generator
		return False

def clean(mode):
	
	generator = get_generator()

	if generator == "Unix Makefiles":

		print 'Cleaning with GNU Make...'
		try_chdir(bin_dir)
		err = os.system(make_cmd + ' clean')
		restore_chdir()

		if err == 0:
			return True
		else:
			print 'GNU Make failed: %s' % err
			return False

	elif generator.startswith('Visual Studio 10'):

		ret = run_vcbuild(generator, mode, '/target:clean')
		
		if ret == 0:
			return True
		else:
			print 'VCBuild failed:', ret
			return False

	elif generator.startswith('Visual Studio 8') or generator.startswith('Visual Studio 9'):

		ret = run_vcbuild(generator, mode, '/clean')
		
		if ret == 0:
			return True
		else:
			print 'VCBuild failed:', ret
			return False

	elif generator == 'Xcode':        

		print 'Cleaning with Xcode...'
		try_chdir(bin_dir)
		err = os.system(xcodebuild_cmd + ' clean')
		restore_chdir()

		if err == 0:
			return True
		else:
			print 'XCode failed:', err
			return False
		
	else:
		print 'clean: Not supported on platform:',sys.platform
		return False

def open_project():
	generator = get_generator()
	if generator.startswith('Visual Studio'):
		open_project_internal(sln_filepath())
		return True
	elif generator.startswith('Xcode'):
		open_project_internal(xcodeproj_filepath(), 'open')
		return True
	else:
		print 'Not supported with generator:',generator
		return False
	
def update():
	print "Running Subversion update..."
	os.system('svn update')
	
def revision():
	# While this doesn't print out the revision specifically, it will do.
	os.system('svn info')

def destroy():
	msg = "Are you sure you want to remove the ./bin/ directory? [y/N]"
	print msg,

	yn = raw_input()
	if yn in ['y', 'Y']:
		try:
			shutil.rmtree(bin_dir)
		except:
			print "Warning: Could not remove ./bin/ directory."

def kill():
	if sys.platform == 'win32':
		os.system('taskkill /F /FI "IMAGENAME eq synergy*"')
		return True
	else:
		print 'kill: Error: Command not implemented for current platform'
		return False
			
def package(type):

	# Package is supported by default.
	package_unsupported = False

	if type == None:
		package_usage()
	elif type == 'src':
		if sys.platform in ['linux2', 'darwin']:
			package_tgz()
		else:
			package_unsupported = True
	elif type == 'rpm':
		if sys.platform == 'linux2':
			package_rpm()
		else:
			package_unsupported = True
	elif type == 'deb':
		if sys.platform == 'linux2':
			package_deb()
		else:
			package_unsupported = True
	elif type == 'win':
		if sys.platform == 'win32':
			package_win()
		else:
			package_unsupported = True
	elif type == 'mac':
		if sys.platform == 'darwin':
			package_mac()
		else:
			package_unsupported = True
	else:
		print 'Not yet implemented: package %s' % type

	if package_unsupported:
		print ('Package type, %s is not '
			'supported for platform, %s') % (type, sys.platform)

def package_tgz():
	try_chdir(bin_dir)
	os.system('make package_source')
	restore_chdir()

def package_rpm():
	try_chdir(bin_dir)
	os.system('cpack -G RPM')
	restore_chdir()

def package_deb():
	try_chdir(bin_dir)
	os.system('cpack -G DEB')
	restore_chdir()

def package_win():
	try_chdir(bin_dir)
	os.system('cpack -G NSIS')
	restore_chdir()

def package_mac():
	try_chdir(bin_dir)
	os.system('cpack -G PackageMaker')
	restore_chdir()

def package_usage():
	print ('Usage: %s package [package-type]\n'
		'\n'
		'Replace [package-type] with one of:\n'
		'  src    .tar.gz source (Posix only)\n'
		'  rpm    .rpm package (Red Hat)\n'
		'  deb    .deb paclage (Debian)\n'
		'  win    .exe installer (Windows)\n'
		'  mac    .dmg package (Mac OS X)\n'
		'\n'
		'Example: %s package src-tgz') % (this_cmd, this_cmd)

def about():
	print ('Help Me script, from the Synergy+ project.\n'
		'%s\n'
		'\n'
		'For help, run: %s help') % (website_url, this_cmd)

#
# Important!
#
# From here on, it's just internal stuff, no need to
# change anything here unless adding new commands, or
# editing the way the script works.
#

def main(argv):

	if sys.version_info < (2, 4):
		print "Python version must be at least: 2.4"
		sys.exit(1)

	if os.path.exists(build_dir):
		# Make sure the user knows the build dir is obsolete.
		os.rename(build_dir, build_dir + '-obsolete')

	completions = []
	arg_1 = ''
	if len(argv) > 1:
		arg_1 = argv[1]
		completions = complete_command(arg_1)

	arg_2 = None
	if len(argv) > 2:
		arg_2 = argv[2]

	if len(completions) > 0:

		if len(completions) == 1:

			cmd = completions[0]

			if cmd != arg_1:
				print 'From `%s`, assuming command `%s`.' % (arg_1, cmd)
				
			if cmd in ['about', 'info']:
				about()
			elif cmd in ['configure', 'conf']:
				configure(arg_2)
			elif cmd in ['build']:
				build(arg_2)
			elif cmd in ['open']:
				open_project()
			elif cmd in ['clean']:
				clean(arg_2)
			elif cmd in ['update', 'up']:
				update()
			elif cmd in ['rev', 'revision']:
				revision()
			elif cmd in ['package', 'dist']:
				package(arg_2)
			elif cmd in ['usage', 'help', '--help', '-h', '/?']:
				usage()
			elif cmd in ['destroy']:
				destroy()
			elif cmd in ['kill']:
				kill()
			elif cmd in ['setup']:
				setup()
			elif cmd in ['hammer']:
				hammer()
			elif cmd in ['reformat']:
				reformat()
			else:
				print 'Command not yet implemented:',cmd

		else:
			print ('Command `%s` too ambiguous, '
				'could mean any of: %s'
				) % (arg_1, ', '.join(completions))
	else:

		if len(argv) == 1:
			print 'No command specified, showing usage.\n'
		else:
			print 'Command not recognised: %s\n' % arg_1

		usage()

def try_chdir(dir):

	# Ensure temp build dir exists.
	if not os.path.exists(dir):
		os.mkdir(dir)

	global prevdir    
	prevdir = os.path.abspath(os.curdir)

	# It will exist by this point, so it's safe to chdir.
	os.chdir(dir)

def restore_chdir():
	global prevdir
	os.chdir(prevdir)

def open_project_internal(project_filename, application = ''):

	if not os.path.exists(project_filename):
		print 'Project file (%s) not found, run hm conf first.' % project_filename
		return False
	else:
		path = project_filename
		if application != '':
			path = application + ' ' + path
		os.system(path)
		return True

def setup(generator = None):
	print "Running setup..."

	# If no generator specified, prompt the user.
	if generator == None:
		if sys.platform == 'win32':
			generator = setup_generator_get(win32_generators)
		elif sys.platform in ['linux2', 'sunos5', 'freebsd7']:
			generator = setup_generator_get(unix_generators)
		elif sys.platform == 'darwin':
			generator = setup_generator_get(darwin_generators)
		else:
			raise Exception('Unsupported platform: ' + sys.platform)

	# Create build dir, since config file resides there.
	if not os.path.exists(bin_dir):
		os.mkdir(bin_dir)

	if os.path.exists(config_filepath()):
		config = ConfigParser.ConfigParser()
		config.read(config_filepath())
	else:
		config = ConfigParser.ConfigParser()

	if not config.has_section('hm'):
		config.add_section('hm')

	if not config.has_section('cmake'):
		config.add_section('cmake')
	
	config.set('hm', 'setup_version', setup_version)
	config.set('cmake', 'generator', generator)

	write_config(config)

	cmakecache_filename = '%s/CMakeCache.txt' % bin_dir
	if os.path.exists(cmakecache_filename):
		print "Removing %s, since generator changed." % cmakecache_filename
		os.remove(cmakecache_filename)

	print "\nSetup complete."

def write_config(config):
	configfile = open(config_filepath(), 'wb')
	config.write(configfile)

def get_generator():
	config = ConfigParser.RawConfigParser()
	config.read(config_filepath())
	return config.get('cmake', 'generator')

def has_setup_version(version):
	if os.path.exists(config_filepath()):
		config = ConfigParser.RawConfigParser()
		config.read(config_filepath())

		try:
			return config.getint('hm', 'setup_version') >= version
		except:
			return False
	else:
		return False

def has_conf_run():
	if has_setup_version(2):
		config = ConfigParser.RawConfigParser()
		config.read(config_filepath())
		try:
			return config.getboolean('hm', 'has_conf_run')
		except:
			return False
	else:
		return False

def set_conf_run():
	if has_setup_version(3):
		config = ConfigParser.RawConfigParser()
		config.read(config_filepath())
		config.set('hm', 'has_conf_run', True)
		write_config(config)
	else:
		raise Exception("User does not have correct setup version.")

def setup_generator_get(generators):

	generator_options = ''
	generators_sorted = sorted(generators.iteritems(), key=lambda t: int(t[0]))
	
	for id, generator in generators_sorted:
		generator_options += '\n  ' + id + ': ' + generator

	print ('\nChoose a CMake generator:%s'
		) % generator_options

	return setup_generator_prompt(generators)

def setup_generator_prompt(generators):

	prompt = 'Enter a number:'
	print prompt,

	generator_id = raw_input()
	
	if generator_id in generators:
		print 'Selected generator:', generators[generator_id]
	else:
		print 'Invalid number, try again.'
		setup_generator_prompt(generators)

	return generators[generator_id]

def complete_command(arg):
	possible_completions = []
	for command in commands:
		if command.startswith(arg):
			possible_completions.append(command)
	return possible_completions

def get_vcvarsall(generator):	
	import platform
	
	# os_bits should be loaded with '32bit' or '64bit'
	(os_bits, other) = platform.architecture()
	
	# visual studio is a 32-bit app, so when we're on 64-bit, we need to check the WoW dungeon
	if os_bits == '64bit':
		key_name = r'SOFTWARE\Wow6432Node\Microsoft\VisualStudio\SxS\VS7'
	else:
		key_name = r'SOFTWARE\Microsoft\VisualStudio\SxS\VC7'
	
	key = _winreg.OpenKey(_winreg.HKEY_LOCAL_MACHINE, key_name)
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

def run_vcbuild(generator, mode, args=''):
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
			) % (get_vcvarsall(generator), vcvars_platform, args, config, config_platform, sln_filepath())
	else:
		config = config + '|' + config_platform
		cmd = ('@echo off\n'
			'call "%s" %s \n'
			'vcbuild /nologo %s "%s" "%s"'
			) % (get_vcvarsall(generator), vcvars_platform, args, sln_filepath(), config)
	print cmd
	# Generate a batch file, since we can't use environment variables directly.
	temp_bat = bin_dir + r'\vcbuild.bat'
	file = open(temp_bat, 'w')
	file.write(cmd)
	file.close()

	return os.system(temp_bat)

def ensure_setup_latest(generator = None):
	if not has_setup_version(setup_version):
		setup(generator)

class HammerBuild:
	generator = None
	target_dir = None
	
	def __init__(self, _generator, _target_dir):
		self.generator = _generator
		self.target_dir = _target_dir
		
	def run(self):
		global bin_dir
		bin_dir = self.target_dir
		configure(self.generator)
		build('debug')
		build('release')

def hammer():
	
	hammer_builds = []
	if sys.platform == 'win32':
		hammer_builds += [
			HammerBuild('Visual Studio 9 2008', 'bin32'),
			HammerBuild('Visual Studio 9 2008 Win64', 'bin64')]
	elif sys.platform in ['linux2', 'sunos5', 'freebsd7']:
		hammer_builds += [
			HammerBuild('Unix Makefiles', 'bin')]
	elif sys.platform == 'darwin':
		hammer_builds += [
			HammerBuild('Xcode', 'bin')]
		
	for hb in hammer_builds:
		hb.run()
		
	package_types = []
	if sys.platform == 'win32':
		package_types += ['win']
	elif sys.platform == 'linux2':
		package_types += ['src', 'rpm', 'deb']
	elif sys.platform == 'darwin':
		package_types += ['mac']
		
	for pt in package_types:
		package(pt)

def reformat():
	# TODO: error handling
	os.system(
		r'tool\astyle\AStyle.exe '
		'--quiet --suffix=none --style=java --indent=force-tab=4 --recursive '
		'lib/*.cpp lib/*.h cmd/*.cpp cmd/*.h')

# Start the program.
main(sys.argv)

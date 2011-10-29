#! /usr/bin/env python

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

# hm.py: 'Help Me', is a simple wrapper for all build tools.
# 
# This script was created for the Synergy project.
# http://synergy-foss.org/
#
# The idea behind this is to simplify the build system,
# however, it's not a dependancy of building Synergy.
# In other words, you don't need to use this script!
#
# If you don't wish to run this script, simply run:
#   cmake .
#   make
# This will create an in-source UNIX Makefile.

import sys, os
sys.path.append('tools')

# if old build src dir exists, move it, as this will now be used for build 
# output.
if os.path.exists('build/toolchain.py'):
	print "Removing legacy build dir."
	os.rename('build', 'build.old')

from build import toolchain
from getopt import gnu_getopt

# minimum required version
requiredMajor = 2
requiredMinor = 3

# options used by all commands
globalOptions = 'v'
globalOptionsLong = ['no-prompts', 'generator=', 'verbose', 'make-gui']

# list of valid commands as keys. the values are optarg strings, but most 
# are None for now (this is mainly for extensibility)
cmd_opt_dict = {
	'about'     : ['', []],
	'setup'     : ['g:', []],
	'configure' : ['g:dr', ['debug', 'release']],
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

def complete_command(arg):
	completions = []
	
	for cmd, optarg in cmd_opt_dict.iteritems():
		# if command was matched fully, return only this, so that
		# if `dist` is typed, it will return only `dist` and not
		# `dist` and `distftp` for example.
		if cmd == arg:
			return [cmd,]
		if cmd.startswith(arg):
			completions.append(cmd)
	
	for alias, cmd in cmd_alias_dict.iteritems():
		# don't know if this will work just like above, but it's
		# probably worth adding.
		if alias == arg:
			return [alias,]
		if alias.startswith(arg):
			completions.append(alias)
	
	return completions

def start_cmd(argv):
	
	cmd_arg = ''
	if len(argv) > 1:
		cmd_arg = argv[1]
			
	# change common help args to help command
	if cmd_arg in ('--help', '-h', '--usage', '-u', '/?'):
		cmd_arg = 'usage'

	completions = complete_command(cmd_arg)
	
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
			if cmd in cmd_alias_dict.keys():
				alias = cmd
				if cmd_arg == cmd:
					cmd_map.append(alias)
				cmd = cmd_alias_dict[cmd]
				cmd_map.append(cmd)
			
			# show command map to avoid confusion
			if len(cmd_map) != 0:
				print 'Mapping command: %s' % ' -> '.join(cmd_map)
			
			run_cmd(cmd, argv[2:])
			
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
		
		run_cmd('usage')
	
	# generic error code if not returned sooner
	return 1

def run_cmd(cmd, argv = []):
	
	verbose = False
	try:
		options_pair = cmd_opt_dict[cmd]
		
		options = globalOptions + options_pair[0]
		
		options_long = []
		options_long.extend(globalOptionsLong)
		options_long.extend(options_pair[1])
		
		opts, args = gnu_getopt(argv, options, options_long)
		
		for o, a in opts:
			if o in ('-v', '--verbose'):
				verbose = True
		
		# pass args and optarg data to command handler, which figures out
		# how to handle the arguments
		handler = toolchain.CommandHandler(argv, opts, args, verbose)
		
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

def main(argv):

	if sys.version_info < (requiredMajor, requiredMinor):
		print ('Python version must be at least ' +
			   str(requiredMajor) + '.' + str(requiredMinor) + ', but is ' +
			   str(sys.version_info[0]) + '.' + str(sys.version_info[1]))
		sys.exit(1)

	try:
		start_cmd(argv)
	except KeyboardInterrupt:
		print '\n\nUser aborted, exiting.'

# Start the program.
main(sys.argv)


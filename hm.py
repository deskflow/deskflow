#! /usr/bin/env python

# hm.py: 'Help Me', is a simple wrapper for all build tools.
# 
# This script was created for the Synergy+ project.
# http://code.google.com/p/synergy-plus
#
# The idea behind this is to simplify the build system,
# however, it's not a dependancy of building Synergy+.
# In other words, you don't need to use this script!
#
# If you don't wish to run this script, simply run:
#   cmake .
#   make
# This will create an in-source UNIX Makefile.

import sys, os
from build import commands

# list of valid commands as keys. the values are optarg strings, but most 
# are None for now (this is mainly for extensibility)
cmd_dict = {
	'about' : [None, []],
	'setup' : [None, []],
	'configure' : [None, []],
	'build' : ['dr', []],
	'clean' : ['dr', []],
	'update' : [None, []],
	'install' : [None, []],
	'package' : [None, []],
	'destroy' : [None, []],
	'kill' : [None, []],
	'usage' : [None, []],
	'revision' : [None, []],
	'hammer' : [None, []],
	'reformat' : [None, []],
	'open'	: [None, []],
}

# aliases to valid commands
cmd_alias_dict = {
	'info'	: 'about',
	'help'	: 'usage',
	'dist'  : 'package',
	'make'	: 'build',
	'cmake'	: 'configure',
}

def complete_command(arg):
	completions = []
	
	for cmd, optarg in cmd_dict.iteritems():
		if cmd.startswith(arg):
			completions.append(cmd)
	
	for alias, cmd in cmd_alias_dict.iteritems():
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

def run_cmd(cmd, args = []):
	# pass args and optarg data to command handler, which figures out
	# how to handle the arguments
	optarg_data = cmd_dict[cmd]
	handler = commands.CommandHandler(args, optarg_data)
	
	# use reflection to get the function pointer
	cmd_func = getattr(handler, cmd)
	cmd_func()

def main(argv):

	if sys.version_info < (2, 4):
		print 'Python version must be at least: 2.4'
		sys.exit(1)

	try:
		sys.exit(start_cmd(argv))
	except KeyboardInterrupt:
		print '\n\nUser aborted, exiting.'

# Start the program.
main(sys.argv)

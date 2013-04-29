# synergy-web -- website for synergy
# Copyright (C) 2012 Bolton Software Ltd.
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

import os, sys, shutil
from getopt import gnu_getopt

importSource = "synergy"
importTarget = "public/locale"
msgfmtFormat = "msgfmt -cv -o %s/website.mo %s/website.po"

def runImport():
	map = {
		"cs-CZ" : "cs",
		"hr-HR" : "hr",
		"hu-HU" : "hu",
		"ja-JP" : "ja",
		"nl-NL" : "nl",
		"pl-PL" : "pl",
		"pt-PT" : "pt",
		"pt-BR" : "pt_BR",
		"sl-SI" : "sl",
		"th-TH" : "th",
		"tr-TR" : "tr",
		"zh-CN" : "zh",
		"zh-TW" : "zh_TW",
		"grk"   : "el",
		"ca-AD" : "ca",
		"pes-IR": "fa",
		"sk-SK" : "sk",
		"bg-BG" : "bg",
		"sq-AL" : "sq"
	}

	for file in os.listdir(importSource):
		mapped = map[file] if file in map else file
		source = "%s/%s/website_%s.po" % (importSource, file, file)
		targetDir = "%s/%s/LC_MESSAGES" % (importTarget, mapped)
		target = targetDir + "/website.po"
		
		print "copy '%s' to '%s'" % (source, target)
		shutil.copy(source, target)
		
		cmd = (msgfmtFormat % (targetDir, targetDir))
		print "run: " + cmd
		os.system(cmd)

def runHelp():
  print ("-h      help\n"
         "-i      import from getlocalization.com\n"
         "-g      generate lang.c file from php")
    
if len(sys.argv) == 1:
	print "usage: lang [-h|-i|-g]"
	sys.exit()

opts, args = gnu_getopt(sys.argv, "hig", "")
for o, a in opts:
	if o == "-h":
		runHelp()
	elif o == "-i":
		runImport()
	elif o == "-g":
		os.system("php -q tsmarty2c.php public\. > public\lang.c")

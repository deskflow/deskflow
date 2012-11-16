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
    "sk-SK" : "sk"
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

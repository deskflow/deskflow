import sys, getopt, subprocess, traceback, os, re

def main():
	try:
		opts, args = getopt.gnu_getopt(sys.argv[1:], "pl:m:v")

		latex_file = None
		metapost_file = None
		preview = False
		verbose = True

		for o, a in opts:
			if o == "-p":
				preview = True
			elif o == "-l":
				latex_file = a
			elif o == "-m":
				metapost_file = a
			elif o == "-v":
				verbose = True
			else:
				raise Exception("unknown option: " + o)
		
		if latex_file:
			latex(latex_file, preview)
		elif metapost_file:
			metapost(metapost_file, preview)
		
	except Exception, err:
		print >> sys.stderr, "error: " + str(err)
		if verbose:
			print "stack trace..."
			traceback.print_tb(sys.exc_traceback)
			print
		usage()
		sys.exit(2)

def latex(filename, preview, build_twice = True, build_mp_files = True):
	dir = os.path.dirname(filename)
	no_ext = ".".join(filename.split('.')[:-1])
	build_time = os.path.getmtime(no_ext + ".log")

	if build_mp_files:
		# build all metapost files first
		mp_files = get_files(dir, build_time, ".*\.mp$", True)
		for f in mp_files:
			metapost(f, False)

	build_latex(filename)

	if build_twice:
		# compile twice to resolve toc, etc
		build_latex(filename)

	if preview:
		start_pdf_preview(filename)
	
def build_latex(filename):
	dir = os.path.dirname(filename)
	p = subprocess.Popen(["pdflatex", filename, "--output-directory=" + dir])
	p.wait()
	print # pdflatex output does not end with \n
	if p.returncode != 0:
		raise Exception("could not build latex file: " + filename)

def start_pdf_preview(filename):
	print "launching preview for: " + filename
	no_ext = ".".join(filename.split('.')[:-1])
	os.startfile(no_ext + ".pdf")

def metapost(filename, preview):
	build_metapost(filename)
	if preview:
		# create a tex file to wrap the compiled metapost
		tex_tmp = mp_to_pdf(filename)

# handy for when `mptopdf` doesn't work (quite common)
def mp_to_pdf(filename):
	no_ext = ".".join(filename.split('.')[:-1])
	no_path = os.path.basename(no_ext)
	tex_tmp = no_ext + ".tex"

	f = open(tex_tmp, "w+")
	f.write("""\\documentclass{article}
\\usepackage[pdftex]{graphicx}
\\DeclareGraphicsRule{*}{mps}{*}{}
\\begin{document}
\\includegraphics{""" + no_path + """.1}
\\end{document}
""")
	f.close()
	
	try:
		# compile the and preview temporary tex
		latex(tex_tmp, True, False, False)
	finally:
		# always delete the wrapper
		os.remove(tex_tmp)

def build_metapost(filename):
	dir = os.path.dirname(filename)
	p = subprocess.Popen(["mpost", filename, "--output-directory=" + dir])
	p.wait()
	print # mpost output does not end with \n
	if p.returncode != 0:
		raise Exception("could not build metapost file: " + filename)

def get_files(dir, after_time, regex, recursive):
	files = os.listdir(dir)
	result = []
	
	for f in files:
		abs = os.path.join(dir, f)
		if os.path.isfile(abs):
			mtime = os.path.getmtime(abs)
			if mtime > after_time and re.match(regex, f):
				result.append(abs)
		elif recursive:
			result += get_files(abs, after_time, regex, True)
	
	return result

def usage():
	print ("build.py usage: "
		   "-l <latex-file>|-m <metapost-file> [-p][-v]")

if __name__ == "__main__":
    main()

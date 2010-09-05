#include <fstream>
#include <iostream>
#include <locale>
#include <set>
#include <string>

using namespace std;

static
string
baseName(const string& filename)
{
	return filename.substr(0, filename.rfind('.'));
}

static
int
writeMakefile(const string& dstdir, const set<string>& depFilenames)
{
	string makeFilename = dstdir + "\\deps.mak";
	ofstream makeFile(makeFilename.c_str());
	if (!makeFile) {
		cerr << "Can't open '" << makeFilename << "' for writing" << endl;
		return 1;
	}

	for (set<string>::const_iterator i = depFilenames.begin();
			i != depFilenames.end(); ++i) {
		makeFile << "!if EXIST(\"" << *i << "\")" << endl;
		makeFile << "!include \"" << *i << "\"" << endl;
		makeFile << "!endif" << endl;
	}

	return 0;
}

static
void
writeDependencies(
	const string& filename,
	const string& srcdir,
	const string& dstdir,
	const set<string>& paths)
{
	string basename = baseName(filename);
	string depFilename = dstdir + "\\" + basename + ".d";
	ofstream depFile(depFilename.c_str());
	if (!depFile) {
		cerr << "Can't open '" << depFilename << "' for writing" << endl;
		return;
	}

	// Write dependencies rule for filename
	depFile << "\"" << dstdir << "\\" << basename << ".obj\": \"" <<
		srcdir << "\\" << filename << "\" \\" << endl;
	for (set<string>::const_iterator i = paths.begin(); i != paths.end(); ++i) {
		depFile << "\t\"" << *i << "\" \\" << endl;
	}
	depFile << "\t$(NULL)" << endl;
}

static
int
writeDepfiles(const string& srcdir, const string& dstdir)
{
	const string includeLine = "Note: including file:";

	// Parse stdin
	string line;
	string filename;
	set<string> paths;
	locale loc = locale::classic();
	const ctype<char>& ct = use_facet<ctype<char> >(loc);
	while (getline(cin, line)) {
		bool echo = true;

		// Check for include line
		if (line.compare(0, includeLine.length(), includeLine) == 0) {
			// Strip includeLine and leading spaces
			line.erase(0, line.find_first_not_of(" ", includeLine.length()));
			if (line.length() == 0) {
				continue;
			}

			// Uppercase all drive letters
			if (line.length() > 2 && line[1] == ':') {
				line[0] = ct.toupper(line[0]);
			}

			// Record path
			paths.insert(line);
			echo = false;
		}

		// Maybe a source filename
		else if (line.rfind(".cpp") == line.length() - 4) {
			// Write dependencies for previous source file
			if (filename.length() != 0) {
				writeDependencies(filename, srcdir, dstdir, paths);
				paths.clear();
			}
			filename = line;
		}

		// Otherwise other output
		else {
			// do nothing
		}

		if (echo) {
			cout << line << endl;
		}
	}

	// Write dependencies for last source file
	if (filename.length() != 0) {
		writeDependencies(filename, srcdir, dstdir, paths);
		paths.clear();
	}

	return 0;
}

int
main(int argc, char** argv)
{
	if (argc < 3) {
		cerr <<  "usage: " << argv[0] <<
			" <src-directory> <dst-directory> [<depfiles>]" << endl;
		return 1;
	}
	string srcdir = argv[1];
	string dstdir = argv[2];

	// If depfiles were supplied then create a makefile in outdir to load
	// all of them.
	int result;
	if (argc > 3) {
		set<string> depFilenames(argv + 3, argv + argc);
		result = writeMakefile(dstdir, depFilenames);
	}

	// Otherwise parse stdin and create a depfile for each listed file
	else {
		result = writeDepfiles(srcdir, dstdir);
	}

	return result;
}

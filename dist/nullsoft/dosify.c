#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static
char*
concatPath(const char* dir, const char* name, const char* ext)
{
	size_t nDir  = (dir != NULL) ? strlen(dir) : 0;
	size_t nPath = nDir + 1 + strlen(name) + strlen(ext?ext:"") + 1;
	char* path = malloc(nPath);

	/* directory */
	if (nDir > 0 && strcmp(dir, ".") != 0) {
		strcpy(path, dir);
		if (path[nDir - 1] != '\\' && path[nDir - 1] != '/') {
			strcat(path, "\\");
		}
	}
	else {
		strcpy(path, "");
	}


	/* name */
	strcat(path, name);

	/* extension */
	if (ext != NULL && strrchr(name, '.') == NULL) {
		strcat(path, ext);
	}

	return path;
}

static
int
dosify(const char* srcdir, const char* dstdir, const char* name)
{
	FILE* dFile, *sFile;
	char* dName, *sName;

	sName = concatPath(srcdir, name, NULL);
	dName = concatPath(dstdir, name, ".txt");

	sFile = fopen(sName, "rb");
	if (sFile == NULL) {
		fprintf(stderr, "Can't open \"%s\" for reading\n", sName);
		return 0;
	}
	else {
		dFile = fopen(dName, "w");
		if (dFile == NULL) {
			fclose(sFile);
			fprintf(stderr, "Can't open \"%s\" for writing\n", dName);
			return 0;
		}
		else {
			char buffer[1024];
			while (!ferror(dFile) &&
				fgets(buffer, sizeof(buffer), sFile) != NULL) {
				fprintf(dFile, "%s", buffer);
			}
			if (ferror(sFile) || ferror(dFile)) {
				fprintf(stderr,
					"Error copying \"%s\" to \"%s\"\n", sName, dName);
				fclose(dFile);
				fclose(sFile);
				_unlink(dName);
				return 0;
			}
		}
	}

	fclose(dFile);
	fclose(sFile);
	free(dName);
	free(sName);
	return 1;
}

#include <windows.h>
int
main(int argc, char** argv)
{
	int i;

	if (argc < 3) {
		fprintf(stderr, "usage: %s <srcdir> <dstdir> [files]\n", argv[0]);
		return 1;
	}

	for (i = 3; i < argc; ++i) {
		if (!dosify(argv[1], argv[2], argv[i]))
			return 1;
	}

	return 0;
}

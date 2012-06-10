/* $XConsortium: pr.c,v 1.17 94/04/17 20:10:38 gildea Exp $ */
/*

Copyright (c) 1993, 1994  X Consortium

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from the X Consortium.

*/

#include "def.h"

extern struct	inclist	inclist[ MAXFILES ],
			*inclistp;
extern char	*objprefix;
extern char	*objsuffix;
extern int	width;
extern boolean	printed;
extern boolean	verbose;
extern boolean	show_where_not;

add_include(filep, file, file_red, include, dot, failOK)
	struct filepointer	*filep;
	struct inclist	*file, *file_red;
	char	*include;
	boolean	dot;
{
	register struct inclist	*newfile;
	register struct filepointer	*content;

	/*
	 * First decide what the pathname of this include file really is.
	 */
	newfile = inc_path(file->i_file, include, dot);
	if (newfile == NULL) {
		if (failOK)
		    return;
		if (file != file_red)
			warning("%s (reading %s, line %d): ",
				file_red->i_file, file->i_file, filep->f_line);
		else
			warning("%s, line %d: ", file->i_file, filep->f_line);
		warning1("cannot find include file \"%s\"\n", include);
		show_where_not = TRUE;
		newfile = inc_path(file->i_file, include, dot);
		show_where_not = FALSE;
	}

	if (newfile) {
		included_by(file, newfile);
		if (!newfile->i_searched) {
			newfile->i_searched = TRUE;
			content = getfile(newfile->i_file);
			find_includes(content, newfile, file_red, 0, failOK);
			freefile(content);
		}
	}
}

recursive_pr_include(head, file, base)
	register struct inclist	*head;
	register char	*file, *base;
{
	register int	i;

	if (head->i_marked)
		return;
	head->i_marked = TRUE;
	if (head->i_file != file)
		pr(head, file, base);
	for (i=0; i<head->i_listlen; i++)
		recursive_pr_include(head->i_list[ i ], file, base);
}

pr(ip, file, base)
	register struct inclist  *ip;
	char	*file, *base;
{
	static char	*lastfile;
	register int	len, i;
	char	buf[ BUFSIZ ];
#ifdef WIN32
	char *transfile = TranslateFileNameD2U(ip->i_file,0);
#else
	char *transfile = ip->i_file;
#endif

	printed = TRUE;
	if (file != lastfile) {
		lastfile = file;
		sprintf(buf, "%s%s%s %s.d: %s", objprefix, base, objsuffix,
			base, transfile);
	}
	else {
		sprintf(buf, " \\\n %s", transfile);
	}
	fwrite(buf, strlen(buf), 1, stdout);

	/*
	 * If verbose is set, then print out what this file includes.
	 */
	if (! verbose || ip->i_list == NULL || ip->i_notified)
		return;
	ip->i_notified = TRUE;
	lastfile = NULL;
	printf("\n# %s includes:", transfile);
	for (i=0; i<ip->i_listlen; i++)
		printf("\n#\t%s", ip->i_list[ i ]->i_incstring);
#ifdef WIN32
	free(transfile);
#endif
}

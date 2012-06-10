/*
 * $XConsortium: ifparser.c,v 1.7 94/01/18 21:30:50 rws Exp $
 *
 * Copyright 1992 Network Computing Devices, Inc.
 * 
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Network Computing Devices may not be
 * used in advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.  Network Computing Devices makes
 * no representations about the suitability of this software for any purpose.
 * It is provided ``as is'' without express or implied warranty.
 * 
 * NETWORK COMPUTING DEVICES DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
 * IN NO EVENT SHALL NETWORK COMPUTING DEVICES BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 * 
 * Author:  Jim Fulton
 *          Network Computing Devices, Inc.
 * 
 * Simple if statement processor
 *
 * This module can be used to evaluate string representations of C language
 * if constructs.  It accepts the following grammar:
 * 
 *     EXPRESSION	:=	VALUE
 * 			 |	VALUE  BINOP	EXPRESSION
 * 
 *     VALUE		:=	'('  EXPRESSION  ')'
 * 			 |	'!'  VALUE
 * 			 |	'-'  VALUE
 * 			 |	'defined'  '('  variable  ')'
 * 			 |	'defined'  variable
 *			 |	# variable '(' variable-list ')'
 * 			 |	variable
 * 			 |	number
 * 
 *     BINOP		:=	'*'	|  '/'	|  '%'
 * 			 |	'+'	|  '-'
 * 			 |	'<<'	|  '>>'
 * 			 |	'<'	|  '>'	|  '<='  |  '>='
 * 			 |	'=='	|  '!='
 * 			 |	'&'	|  '|'
 * 			 |	'&&'	|  '||'
 * 
 * The normal C order of precidence is supported.
 * 
 * 
 * External Entry Points:
 * 
 *     ParseIfExpression		parse a string for #if
 */

#include "ifparser.h"
#include <ctype.h>

/****************************************************************************
		   Internal Macros and Utilities for Parser
 ****************************************************************************/

#define DO(val) if (!(val)) return NULL
#define CALLFUNC(ggg,fff) (*((ggg)->funcs.fff))
#define SKIPSPACE(ccc) while (isspace(*ccc)) ccc++
#define isvarfirstletter(ccc) (isalpha(ccc) || (ccc) == '_')


static const char *
parse_variable (g, cp, varp)
    IfParser *g;
    const char *cp;
    const char **varp;
{
    SKIPSPACE (cp);

    if (!isvarfirstletter (*cp))
	return CALLFUNC(g, handle_error) (g, cp, "variable name");

    *varp = cp;
    /* EMPTY */
    for (cp++; isalnum(*cp) || *cp == '_'; cp++) ;
    return cp;
}


static const char *
parse_number (g, cp, valp)
    IfParser *g;
    const char *cp;
    int *valp;
{
    SKIPSPACE (cp);

    if (!isdigit(*cp))
	return CALLFUNC(g, handle_error) (g, cp, "number");

    *valp = strtol(cp, &cp, 0);
    return cp;
}


static const char *
parse_value (g, cp, valp)
    IfParser *g;
    const char *cp;
    int *valp;
{
    const char *var;

    *valp = 0;

    SKIPSPACE (cp);
    if (!*cp)
	return cp;

    switch (*cp) {
      case '(':
	DO (cp = ParseIfExpression (g, cp + 1, valp));
	SKIPSPACE (cp);
	if (*cp != ')') 
	    return CALLFUNC(g, handle_error) (g, cp, ")");

	return cp + 1;			/* skip the right paren */

      case '!':
	DO (cp = parse_value (g, cp + 1, valp));
	*valp = !(*valp);
	return cp;

      case '-':
	DO (cp = parse_value (g, cp + 1, valp));
	*valp = -(*valp);
	return cp;

      case '#':
	DO (cp = parse_variable (g, cp + 1, &var));
	SKIPSPACE (cp);
	if (*cp != '(')
	    return CALLFUNC(g, handle_error) (g, cp, "(");
	do {
	    DO (cp = parse_variable (g, cp + 1, &var));
	    SKIPSPACE (cp);
	} while (*cp && *cp != ')');
	if (*cp != ')')
	    return CALLFUNC(g, handle_error) (g, cp, ")");
	*valp = 1; /* XXX */
	return cp + 1;

      case 'd':
	if (strncmp (cp, "defined", 7) == 0 && !isalnum(cp[7])) {
	    int paren = 0;
	    cp += 7;
	    SKIPSPACE (cp);
	    if (*cp == '(') {
		paren = 1;
		cp++;
	    }
	    DO (cp = parse_variable (g, cp, &var));
	    SKIPSPACE (cp);
	    if (paren && *cp != ')')
		return CALLFUNC(g, handle_error) (g, cp, ")");
	    *valp = (*(g->funcs.eval_defined)) (g, var, cp - var);
	    return cp + paren;		/* skip the right paren */
	}
	/* fall out */
    }

    if (isdigit(*cp)) {
	DO (cp = parse_number (g, cp, valp));
    } else if (!isvarfirstletter(*cp))
	return CALLFUNC(g, handle_error) (g, cp, "variable or number");
    else {
	DO (cp = parse_variable (g, cp, &var));
	*valp = (*(g->funcs.eval_variable)) (g, var, cp - var);
    }
    
    return cp;
}



static const char *
parse_product (g, cp, valp)
    IfParser *g;
    const char *cp;
    int *valp;
{
    int rightval;

    DO (cp = parse_value (g, cp, valp));
    SKIPSPACE (cp);

    switch (*cp) {
      case '*':
	DO (cp = parse_product (g, cp + 1, &rightval));
	*valp = (*valp * rightval);
	break;

      case '/':
	DO (cp = parse_product (g, cp + 1, &rightval));
	*valp = (*valp / rightval);
	break;

      case '%':
	DO (cp = parse_product (g, cp + 1, &rightval));
	*valp = (*valp % rightval);
	break;
    }
    return cp;
}


static const char *
parse_sum (g, cp, valp)
    IfParser *g;
    const char *cp;
    int *valp;
{
    int rightval;

    DO (cp = parse_product (g, cp, valp));
    SKIPSPACE (cp);

    switch (*cp) {
      case '+':
	DO (cp = parse_sum (g, cp + 1, &rightval));
	*valp = (*valp + rightval);
	break;

      case '-':
	DO (cp = parse_sum (g, cp + 1, &rightval));
	*valp = (*valp - rightval);
	break;
    }
    return cp;
}


static const char *
parse_shift (g, cp, valp)
    IfParser *g;
    const char *cp;
    int *valp;
{
    int rightval;

    DO (cp = parse_sum (g, cp, valp));
    SKIPSPACE (cp);

    switch (*cp) {
      case '<':
	if (cp[1] == '<') {
	    DO (cp = parse_shift (g, cp + 2, &rightval));
	    *valp = (*valp << rightval);
	}
	break;

      case '>':
	if (cp[1] == '>') {
	    DO (cp = parse_shift (g, cp + 2, &rightval));
	    *valp = (*valp >> rightval);
	}
	break;
    }
    return cp;
}


static const char *
parse_inequality (g, cp, valp)
    IfParser *g;
    const char *cp;
    int *valp;
{
    int rightval;

    DO (cp = parse_shift (g, cp, valp));
    SKIPSPACE (cp);

    switch (*cp) {
      case '<':
	if (cp[1] == '=') {
	    DO (cp = parse_inequality (g, cp + 2, &rightval));
	    *valp = (*valp <= rightval);
	} else {
	    DO (cp = parse_inequality (g, cp + 1, &rightval));
	    *valp = (*valp < rightval);
	}
	break;

      case '>':
	if (cp[1] == '=') {
	    DO (cp = parse_inequality (g, cp + 2, &rightval));
	    *valp = (*valp >= rightval);
	} else {
	    DO (cp = parse_inequality (g, cp + 1, &rightval));
	    *valp = (*valp > rightval);
	}
	break;
    }
    return cp;
}


static const char *
parse_equality (g, cp, valp)
    IfParser *g;
    const char *cp;
    int *valp;
{
    int rightval;

    DO (cp = parse_inequality (g, cp, valp));
    SKIPSPACE (cp);

    switch (*cp) {
      case '=':
	if (cp[1] == '=')
	    cp++;
	DO (cp = parse_equality (g, cp + 1, &rightval));
	*valp = (*valp == rightval);
	break;

      case '!':
	if (cp[1] != '=')
	    break;
	DO (cp = parse_equality (g, cp + 2, &rightval));
	*valp = (*valp != rightval);
	break;
    }
    return cp;
}


static const char *
parse_band (g, cp, valp)
    IfParser *g;
    const char *cp;
    int *valp;
{
    int rightval;

    DO (cp = parse_equality (g, cp, valp));
    SKIPSPACE (cp);

    switch (*cp) {
      case '&':
	if (cp[1] != '&') {
	    DO (cp = parse_band (g, cp + 1, &rightval));
	    *valp = (*valp & rightval);
	}
	break;
    }
    return cp;
}


static const char *
parse_bor (g, cp, valp)
    IfParser *g;
    const char *cp;
    int *valp;
{
    int rightval;

    DO (cp = parse_band (g, cp, valp));
    SKIPSPACE (cp);

    switch (*cp) {
      case '|':
	if (cp[1] != '|') {
	    DO (cp = parse_bor (g, cp + 1, &rightval));
	    *valp = (*valp | rightval);
	}
	break;
    }
    return cp;
}


static const char *
parse_land (g, cp, valp)
    IfParser *g;
    const char *cp;
    int *valp;
{
    int rightval;

    DO (cp = parse_bor (g, cp, valp));
    SKIPSPACE (cp);

    switch (*cp) {
      case '&':
	if (cp[1] != '&')
	    return CALLFUNC(g, handle_error) (g, cp, "&&");
	DO (cp = parse_land (g, cp + 2, &rightval));
	*valp = (*valp && rightval);
	break;
    }
    return cp;
}


static const char *
parse_lor (g, cp, valp)
    IfParser *g;
    const char *cp;
    int *valp;
{
    int rightval;

    DO (cp = parse_land (g, cp, valp));
    SKIPSPACE (cp);

    switch (*cp) {
      case '|':
	if (cp[1] != '|')
	    return CALLFUNC(g, handle_error) (g, cp, "||");
	DO (cp = parse_lor (g, cp + 2, &rightval));
	*valp = (*valp || rightval);
	break;
    }
    return cp;
}


/****************************************************************************
			     External Entry Points
 ****************************************************************************/

const char *
ParseIfExpression (g, cp, valp)
    IfParser *g;
    const char *cp;
    int *valp;
{
    return parse_lor (g, cp, valp);
}



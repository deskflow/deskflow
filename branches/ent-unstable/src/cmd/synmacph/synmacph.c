/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2014 Bolton Software Ltd.
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <syslog.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, const char * argv[])
{
    #pragma unused(argc)
    #pragma unused(argv)

	system("sudo sqlite3 /Library/Application\\ Support/com.apple.TCC/TCC.db 'delete from access where client like \"%Synergy.app%\"'");
	
	(void) sleep(10);
	
	return EXIT_SUCCESS;
}


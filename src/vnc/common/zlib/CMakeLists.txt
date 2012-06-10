# synergy -- mouse and keyboard sharing utility
# Copyright (C) 2012 Nick Bolton
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

set(inc
	deflate.h
	infblock.h
	infcodes.h
	inffast.h
	inffixed.h
	inftrees.h
	infutil.h
	trees.h
	zconf.h
	zlib.h
	zutil.h
)

set(src
	adler32.c
	compress.c
	crc32.c
	deflate.c
	gzio.c
	infblock.c
	infcodes.c
	inffast.c
	inflate.c
	inftrees.c
	infutil.c
	minigzip.c
	trees.c
	uncompr.c
	zutil.c
)

if (WIN32)
	list(APPEND src ${inc})
endif()

include_directories(${inc})
add_library(vnc_zlib STATIC ${src})

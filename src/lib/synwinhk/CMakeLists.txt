# synergy -- mouse and keyboard sharing utility
# Copyright (C) 2013 Bolton Software Ltd.
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

set(headers
	synwinhk.h
)

set(source
	synwinhk.cpp
)

list(APPEND source
	${headers}
)

set(includes
	../common
	../synergy
)

include_directories(${includes})
add_library(synwinhk SHARED ${source})

# copy the dlls (and supporting files) from the lib dir to 
# the bin dir, so that synergyc and synergys can easily find them.
# we should leave the other libraries compiling to the lib dir,
# so that the bin dir remains tidy. the path is relative to the
# build dir (in this case, that's: build\src\lib\platform).
add_custom_command(
	TARGET synwinhk 
	POST_BUILD
	COMMAND xcopy /Y /Q
		..\\..\\..\\..\\lib\\${CMAKE_CFG_INTDIR}\\synwinhk.*
		..\\..\\..\\..\\bin\\${CMAKE_CFG_INTDIR}\\
)

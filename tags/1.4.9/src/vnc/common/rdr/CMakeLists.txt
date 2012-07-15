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
	Exception.h
	FdInStream.h
	FdOutStream.h
	FixedMemOutStream.h
	HexInStream.h
	HexOutStream.h
	InStream.h
	MemInStream.h
	MemOutStream.h
	OutStream.h
	RandomStream.h
	SubstitutingInStream.h
	types.h
	ZlibInStream.h
	ZlibOutStream.h
)

set(src
	Exception.cxx
	FdInStream.cxx
	FdOutStream.cxx
	HexInStream.cxx
	HexOutStream.cxx
	InStream.cxx
	RandomStream.cxx
	ZlibInStream.cxx
	ZlibOutStream.cxx
)

if (WIN32)
	list(APPEND src ${inc})
endif()

set(inc
	..
	../zlib
)

include_directories(${inc})
add_library(vnc_rdr STATIC ${src})
target_link_libraries(vnc_rdr
	vnc_zlib)

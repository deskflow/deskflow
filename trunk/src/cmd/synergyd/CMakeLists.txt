# synergy -- mouse and keyboard sharing utility
# Copyright (C) 2012 Bolton Software Ltd.
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

set(src
	synergyd.cpp
)

set(inc
	../../lib/arch
	../../lib/base
	../../lib/common
	../../lib/io
	../../lib/ipc
	../../lib/mt
	../../lib/net
	../../lib/platform
	../../lib/synergy
)

if (UNIX)
	list(APPEND inc
		../../..
	)
endif()

include_directories(${inc})

if (WIN32)
  add_executable(synergyd WIN32 ${src})
else()
  add_executable(synergyd ${src})
endif()

target_link_libraries(synergyd
	arch base common io ipc mt net platform synergy cryptopp ${libs})

if (CONF_CPACK)
	install(TARGETS
		synergyd
		COMPONENT core
		DESTINATION bin)
endif()

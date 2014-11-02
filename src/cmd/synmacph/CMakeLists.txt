# synergy -- mouse and keyboard sharing utility
# Copyright (C) 2014 Synergy Si Ltd.
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

file(GLOB sources "*.c" "*.plist")

include_directories(
	../
)

add_executable(synmacph ${sources})

get_target_property(current_property synmacph LINK_FLAGS)

set(OTHER_LINK_FLAGS "-sectcreate __TEXT __info_plist ${root_dir}/src/cmd/synmacph/Info.plist -sectcreate __TEXT __launchd_plist ${root_dir}/src/cmd/synmacph/Launchd.plist")

if (NOT ${current_property})
	set_target_properties(synmacph PROPERTIES LINK_FLAGS ${OTHER_LINK_FLAGS})
endif()

target_link_libraries(synmacph)

if (CONF_CPACK)
	install(TARGETS
		synmacph
		COMPONENT core
		DESTINATION bin)
endif()
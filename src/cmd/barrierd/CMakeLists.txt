# barrier -- mouse and keyboard sharing utility
# Copyright (C) 2012-2016 Symless Ltd.
# Copyright (C) 2012 Nick Bolton
#
# This package is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# found in the file LICENSE that should have accompanied this file.
#
# This package is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

file(GLOB headers "*.h")
file(GLOB sources "*.cpp")
if (WIN32)
    list(APPEND sources barrierd.rc)
endif()

if (WIN32)
  add_executable (barrierd WIN32 ${sources})
else()
  add_executable (barrierd ${sources})
endif()

target_link_libraries (barrierd
    arch base common io ipc mt net platform synlib ${libs} ${OPENSSL_LIBS})

# Deskflow -- mouse and keyboard sharing utility
# Copyright (C) 2024 Symless Ltd.
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

import os, fnmatch


def find_files(search_dirs, include_files, exclude_dirs=[]):
    """Recursively find files, excluding specified directories"""
    matches = []
    for dir in search_dirs:
        for root, dirnames, filenames in os.walk(dir):
            dirnames[:] = [d for d in dirnames if d not in exclude_dirs]

            for pattern in include_files:
                for filename in fnmatch.filter(filenames, pattern):
                    matches.append(os.path.join(root, filename))
    return matches

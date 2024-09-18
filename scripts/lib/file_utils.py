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

import glob, os, shutil, sys
import lib.env as env
import lib.colors as colors


class CopyOptions:
    def __init__(self, ignore_errors, verbose):
        self.ignore_errors = ignore_errors
        self.verbose = verbose


class CopyContext:
    def __init__(self):
        self.errors = 0
        self.permission_error = False


def copy(source, target, options):
    """Copy files and directories from source to target."""

    context = CopyContext()
    if options.verbose:
        print(f"Copying files from {source} to {target}")

    try:
        for match in glob.glob(source):

            if os.path.isfile(match):
                copy_file(match, target, options, context)
            elif os.path.isdir(match):
                copy_dir(match, target, options, context)
            else:
                raise RuntimeError(f"Path {match} is not a file or directory")
    finally:
        if context.errors and options.ignore_errors:
            print(f"{colors.WARNING_TEXT} Ignored {context.errors} copy error(s)")

        if context.permission_error and env.is_windows():
            print(
                f"{colors.HINT_TEXT} A Windows file permission error may mean that the file is in use"
            )


def copy_dir(match, target, options, context):
    if options.verbose:
        print(f"Copying directory {match} to {target}")

    try:
        shutil.copytree(match, target, dirs_exist_ok=True)
    except Exception as e:
        handle_all_copy_errors(e, options, context)


def copy_file(match, target, options, context):
    if options.verbose:
        print(f"Copying file {match} to {target}")

    try:
        shutil.copy(match, target)
    except Exception as e:
        handle_all_copy_errors(e, options, context)


def handle_all_copy_errors(error, options, context):

    if isinstance(error, shutil.Error):
        for _, _, file_error in error.args[0]:
            handle_copy_error(file_error, options, context)
    else:
        handle_copy_error(error, options, context)

    if not options.ignore_errors:
        raise error


def handle_copy_error(error, options, context):
    if isinstance(error, PermissionError):
        context.permission_error = True

    if isinstance(error, str):
        context.permission_error = error.startswith("[Errno 13] Permission denied")

    context.errors += 1

    if options.ignore_errors:
        print(
            f"{colors.WARNING_TEXT} Copy failed: {error}",
            file=sys.stderr,
        )

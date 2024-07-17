import glob, os, shutil, sys

import colorama  # type: ignore
from colorama import Fore  # type: ignore

colorama.init()


class CopyOptions:
    def __init__(self, ignore_errors, verbose):
        self.ignore_errors = ignore_errors
        self.verbose = verbose


class CopyContext:
    def __init__(self):
        self.permission_error = False


def copy(source, target, options):
    """Copy files and directories from source to target."""

    context = CopyContext()
    if options.verbose:
        print(f"Copying files from {source} to {target}")
    for match in glob.glob(source):

        if os.path.isfile(match):
            copy_file(match, target, options, context)
        elif os.path.isdir(match):
            copy_dir(match, target, options, context)
        else:
            raise RuntimeError(f"Path {match} is not a file or directory")

    if context.permission_error:
        print(
            f"{Fore.BLUE}HINT:{Fore.RESET} A permission error may mean that the file is in use"
        )


def copy_dir(match, target, options, context):
    if options.verbose:
        print(f"Copying directory {match} to {target}")

    try:
        shutil.copytree(match, target, dirs_exist_ok=True)

    except PermissionError as e:
        handle_permission_error(e, options)
        context.permission_error = True
    except Exception as e:
        handle_copy_error(e, options)


def copy_file(match, target, options, context):
    if options.verbose:
        print(f"Copying file {match} to {target}")

    try:
        shutil.copy(match, target)
    except PermissionError as e:
        handle_permission_error(e, options)
        context.permission_error = True
    except Exception as e:
        handle_copy_error(e, options)


def handle_copy_error(e, options):
    if not options.ignore_errors:
        raise e
    else:
        print(f"{Fore.YELLOW}WARNING:{Fore.RESET} Copy failed: {e}", file=sys.stderr)


def handle_permission_error(e, options):
    if not options.ignore_errors:
        raise e
    else:
        print(
            f"{Fore.YELLOW}WARNING:{Fore.RESET} Copy failed: {e}",
            file=sys.stderr,
        )

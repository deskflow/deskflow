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

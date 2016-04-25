#! /bin/bash

pybin=python
# Use python2 explicitly if available
# otherwise things break on arch or any
# distro that has python3 by default
py2=$(which python2 2> /dev/null)
if [ $? = 0 ]; then
	pybin=$py2
fi
$pybin hm.py "$@"

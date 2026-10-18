#!/bin/zsh
set -eu
smooth_dir=${0:A:h}
exec /usr/bin/sudo "$smooth_dir/smooth-setup" uninstall

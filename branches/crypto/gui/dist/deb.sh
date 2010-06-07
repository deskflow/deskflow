#!/bin/bash

cd $(dirname $0)/..

dist/tarball.sh

rm -rf tmp/debian
mkdir tmp/debian
cd tmp/debian
tar xfz ../../qsynergy*.tar.gz
cd qsynergy*
ln -s dist/debian debian
dpkg-buildpackage -sgpg -rfakeroot


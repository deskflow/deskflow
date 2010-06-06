#!/bin/bash

APPNAME=qsynergy
VERSION=0.9.0
TMPBASE=/tmp
TMPDIR=${TMPBASE}/${APPNAME}-${VERSION}

cd $(dirname $0)/..
[ -e Makefile ] && make distclean
mkdir -p ${TMPDIR}
cp -a src/ ${TMPDIR}
cp -a res/ ${TMPDIR}
cp -a dist/ ${TMPDIR}
cp qsynergy.pro COPYING INSTALL README ${TMPDIR}
tar cCfz ${TMPBASE} ${APPNAME}-${VERSION}.tar.gz --exclude=.svn ${APPNAME}-${VERSION}
rm -rf ${TMPDIR}


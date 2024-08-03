#!/usr/bin/env bash

. ./tmp/rhel_login.sh

subscription-manager register --username $RHEL_USERNAME --password $RHEL_PASSWORD \
  && yum repolist \
  && subscription-manager attach --auto \
  && subscription-manager repos --enable=$RHEL_REPOS \
  && yum repolist

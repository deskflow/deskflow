#!/usr/bin/env bash

if [ "$1" == "--deactivate" ]; then

  subscription-manager unregister
  subscription-manager clean
  echo "RHEL subscription deactivated and credentials removed."
  exit 0

fi

if [ "$1" == "--activate" ]; then

  . ./tmp/rhel_login.sh

  subscription-manager register --username $RHEL_USERNAME --password $RHEL_PASSWORD \
    && yum repolist \
    && subscription-manager attach --auto \
    && subscription-manager repos --enable=$RHEL_REPOS \
    && yum repolist

  echo "RHEL subscription activated and repositories enabled."
  exit 0
fi

echo "Usage: $0 [--activate | --deactivate]"
exit 1

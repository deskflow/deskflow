ARG BASE_IMAGE
FROM $BASE_IMAGE AS base

RUN zypper refresh && zypper update -y && zypper install -y git python3 && zypper clean --all

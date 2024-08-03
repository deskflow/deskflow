# syntax=docker/dockerfile:1

ARG BASE_IMAGE
FROM $BASE_IMAGE AS base

FROM base AS deps

ARG RHEL_USERNAME
ARG RHEL_PASSWORD

ENV RHEL_USERNAME=$RHEL_USERNAME
ENV RHEL_PASSWORD=$RHEL_PASSWORD
ENV RHEL_REPOS="codeready-builder-for-rhel-8-x86_64-rpms"

WORKDIR /app

RUN --mount=type=bind,target=/app,r \
  ./scripts/activate_rhel.sh

RUN dnf upgrade -y && \
  dnf install -y git python3 && \
  dnf clean all

RUN --mount=type=bind,target=/app,rw \
  ./scripts/install_deps.py --ci-env && \
  dnf clean all

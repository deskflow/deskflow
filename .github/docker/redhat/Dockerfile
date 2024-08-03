# syntax=docker/dockerfile:1

ARG BASE_IMAGE
FROM $BASE_IMAGE AS base

FROM base AS deps

ENV RHEL_REPOS="codeready-builder-for-rhel-9-x86_64-rpms"

WORKDIR /app

RUN --mount=type=bind,target=/app,rw \
  ./scripts/activate_rhel.sh

RUN dnf upgrade -y && \
  dnf install -y git python3 && \
  dnf clean all

RUN --mount=type=bind,target=/app,rw \
  ./scripts/install_deps.py --ci-env && \
  dnf clean all

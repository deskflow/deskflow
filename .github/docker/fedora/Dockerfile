# syntax=docker/dockerfile:1

ARG BASE_IMAGE
FROM $BASE_IMAGE AS base

FROM base AS deps

RUN dnf upgrade -y && \
  dnf install -y git python3 && \
  dnf clean all

WORKDIR /app

RUN --mount=type=bind,target=/app,rw \
  ./scripts/install_deps.py && \
  dnf clean all

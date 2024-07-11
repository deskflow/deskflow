# syntax=docker/dockerfile:1

ARG BASE_IMAGE
FROM $BASE_IMAGE AS base

FROM base AS deps

RUN zypper refresh && \
  zypper update -y && \
  zypper install -y git python3 && \
  zypper clean --all

WORKDIR /app

RUN --mount=type=bind,target=/app,rw \
  ./scripts/install_deps.py && \
  zypper clean --all

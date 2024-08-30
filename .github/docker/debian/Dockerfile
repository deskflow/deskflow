# syntax=docker/dockerfile:1

ARG BASE_IMAGE
FROM $BASE_IMAGE AS base

FROM base AS deps

RUN apt update && \
  apt dist-upgrade -y && \
  apt install -y git python3 && \
  apt clean

WORKDIR /app

RUN --mount=type=bind,target=/app,rw \
  ./scripts/install_deps.py && \
  apt clean

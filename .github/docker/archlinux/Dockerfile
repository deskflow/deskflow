ARG BASE_IMAGE
FROM $BASE_IMAGE AS base

FROM base AS deps

RUN pacman -Syu --noconfirm git python sudo && \
  pacman -Scc --noconfirm

RUN useradd -m build

WORKDIR /app

RUN --mount=type=bind,target=/app,rw \
  ./scripts/install_deps.py && \
  pacman -Scc --noconfirm

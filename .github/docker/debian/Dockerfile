# syntax=docker/dockerfile:1

ARG BASE_IMAGE
FROM $BASE_IMAGE AS base

FROM base AS deps
RUN apt update && apt dist-upgrade -y && apt install -y git python3 && apt clean
WORKDIR /scripts
RUN --mount=type=bind,target=.,rw && ./scripts/install_deps.sh && apt clean

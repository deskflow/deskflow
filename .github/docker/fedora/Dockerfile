ARG DOCKER_BASE_IMAGE
FROM $DOCKER_BASE_IMAGE AS base

RUN dnf upgrade -y && dnf install -y git python3 && dnf clean all

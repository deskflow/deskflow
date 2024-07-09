ARG DOCKER_BASE_IMAGE
FROM $DOCKER_BASE_IMAGE AS base

RUN apt update && apt dist-upgrade -y && apt install -y git python3 && apt clean

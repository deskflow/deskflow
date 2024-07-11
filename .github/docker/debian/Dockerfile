ARG BASE_IMAGE
FROM $BASE_IMAGE AS base

RUN apt update && apt dist-upgrade -y && apt install -y git python3 && apt clean

COPY scripts /

RUN ./install_deps.py

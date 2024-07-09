FROM --platform=$BUILDPLATFORM debian:12 AS base

RUN apt update && apt dist-upgrade -y && apt install -y git python3 && apt clean

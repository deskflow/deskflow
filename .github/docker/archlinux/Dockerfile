ARG BASE_IMAGE
FROM $BASE_IMAGE AS base

RUN pacman -Syu --noconfirm git python sudo

RUN useradd -m build

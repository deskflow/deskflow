FROM ubuntu:xenial

RUN apt-get update && apt-get install -qy apt-utils locales
RUN locale-gen --no-purge en_US.UTF-8
ENV LC_ALL en_US.UTF-8

RUN apt install -y qtcreator qtbase5-dev cmake make g++ xorg-dev libssl-dev libx11-dev libsodium-dev libgl1-mesa-glx libegl1-mesa libcurl4-openssl-dev libavahi-compat-libdnssd-dev\
        qtdeclarative5-dev libqt5svg5-dev libsystemd-dev git

ENV BOOST_ROOT=/root/boost

COPY . /home/synergy-core

RUN mkdir -p /home/synergy-core/build

WORKDIR /home/synergy-core/build


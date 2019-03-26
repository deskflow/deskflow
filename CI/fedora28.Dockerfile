# Pull base image.
FROM fedora:28

# Install.
RUN \
  yum -y groupinstall "Development Tools" && \
  yum -y install avahi-compat-libdns_sd-devel avahi-compat-libdns_sd cmake3 boost-static git libXtst-devel qt5-qtbase-devel qt5-qtdeclarative-devel libcurl-devel openssl-devel
  
  
# Set environment variables.
ENV HOME /root

# Define working directory.
WORKDIR /root

# Define default command.
CMD ["bash"]
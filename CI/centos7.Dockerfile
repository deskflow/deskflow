# Pull base image.
FROM centos:7

# Install.
RUN \
  yum -y groupinstall "Development Tools" && \
  yum -y install epel-release cmake3 boost-static git libXtst-devel qt5-qtbase-devel qt5-qtdeclarative-devel libcurl-devel openssl-devel

  
  
# Set environment variables.
ENV HOME /root

# Define working directory.
WORKDIR /root

# Define default command.
CMD ["bash"]
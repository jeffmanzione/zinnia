FROM gcc:10.2.0
WORKDIR /usr/src
EXPOSE 80
EXPOSE 443

# Installs basic tools for C development + some extras.
RUN \
  apt-get update && \
  apt-get -y install \
  curl \
  gnupg \
  gpg \
  less \
  libc6 \
  patch \
  python \
  python-pkg-resources \
  software-properties-common \
  unzip \
  ssh-client \
  lsof

# Install Git >2.0.1
RUN \
  apt-get -y update && \
  apt-get -y install git

# Install bazel
RUN \
  curl -fsSL https://bazel.build/bazel-release.pub.gpg | gpg --dearmor > bazel.gpg && \
  mv bazel.gpg /etc/apt/trusted.gpg.d/ && \
  echo "deb [arch=amd64] https://storage.googleapis.com/bazel-apt stable jdk1.8" | tee /etc/apt/sources.list.d/bazel.list && \
  apt-get update && \
  apt-get -y install bazel && \
  apt-get -y upgrade bazel

# Development environment for building Zinnia.
#
#   docker build -t zinnia-dev zinnia/docker
#   docker run -it -v "$PWD":/usr/src/zinnia -w /usr/src/zinnia zinnia-dev
#   bazel test -c opt //...
FROM gcc:14
WORKDIR /usr/src
EXPOSE 80
EXPOSE 443

# Installs basic tools for C development + some extras.
RUN \
  apt-get update && \
  apt-get -y install --no-install-recommends \
  ca-certificates \
  curl \
  git \
  less \
  lsof \
  openssh-client \
  patch \
  python3 \
  unzip && \
  rm -rf /var/lib/apt/lists/*

# Installs Bazelisk as `bazel`, which uses the version in .bazelversion.
ARG BAZELISK_VERSION=v1.25.0
ARG TARGETARCH=amd64
RUN \
  curl -fsSL -o /usr/local/bin/bazel \
  "https://github.com/bazelbuild/bazelisk/releases/download/${BAZELISK_VERSION}/bazelisk-linux-${TARGETARCH}" && \
  chmod +x /usr/local/bin/bazel

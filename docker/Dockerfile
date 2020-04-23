ARG UBUNTU_VERSION=bionic-20200311

FROM ubuntu:${UBUNTU_VERSION} as base

# We don't care for __pycache__ and .pyc files; sometimes VSCode doesn't clean
# up properly when deleting things and the cache gets stale.
ENV PYTHONDONTWRITEBYTECODE 1

# python3
RUN apt-get update && \
    apt-get install -y \
        python3-dev \
        python3-pip
RUN pip3 install pytest

# build-essential-ish with clang; forcing to libc++ 8
RUN apt-get update && \
    apt-get install -y \
        clang-8 \
        cmake \
        dpkg-dev \
        g++-8 \
        gcc-8 \
        gdb \
        libc6-dev \
        libc++-8-dev \
        libc++abi-8-dev \
        libstdc++-8-dev

RUN \
    update-alternatives --install /usr/bin/cc cc /usr/bin/clang-8 100 && \
    update-alternatives --install /usr/bin/c++ c++ /usr/bin/clang++-8 100

# Dev tools
RUN \
    apt-get update && \
    apt-get install -y \
        less \
        unzip \
        wget \
        vim

# Gtest
RUN \
    cd /tmp && \
    wget https://github.com/google/googletest/archive/release-1.10.0.tar.gz && \
    tar xfz release-1.10.0.tar.gz && \
    mv googletest-release-1.10.0 /opt/gtest && \
    cd /opt/gtest && \
    cmake -DCMAKE_CXX_FLAGS="-fPIC -std=c++17 -stdlib=libc++ -O3 -g" . && \
    make -j `nproc`


# Protobuf
ARG PROTOBUF_ROOT=c++/external/protobuf
RUN apt-get update && apt-get install -y autoconf libtool zlib1g-dev
COPY ${PROTOBUF_ROOT} /opt/protobuf
RUN \
    cd /opt/protobuf && \
    rm -rf /opt/protobuf/third_party/googletest && \
    ln -s /opt/gtest /opt/protobuf/third_party/googletest && \
    mkdir -p build && cd build && \
    cmake -DCMAKE_CXX_FLAGS="-fPIC -std=c++17 -stdlib=libc++ -O3 -g -Wno-deprecated-declarations" ../cmake/ && \
    make -j `nproc` && \
    make check && \
    make install && \
    cd /opt/protobuf/python && \
    python3 setup.py install


# Libarchive
ARG LIBARCHIVE_ROOT=c++/external/libarchive
COPY ${LIBARCHIVE_ROOT} /opt/libarchive
RUN \
    cd /opt/libarchive && \
    mkdir -p build && cd build && \
    cmake .. && \
    make -j `nproc` && \
    make test && \
    make install


# fmt (To avoid requiring c++20 for now)
ARG FMT_ROOT=c++/external/fmt_lib
COPY ${FMT_ROOT} /opt/fmt
RUN \
    cd /opt/fmt && \
    mkdir -p build && cd build && \
    cmake .. && \
    make -j `nproc` && \
    make test && \
    make install


# Include a build of protobag
COPY ./c++/CMakeLists.txt /opt/protobag/CMakeLists.txt
COPY ./c++/protobag /opt/protobag/protobag
COPY ./c++/protobag_test /opt/protobag/protobag_test
WORKDIR /opt/protobag
RUN \
    mkdir -p build && cd build && \
    cmake .. && \
    make -j `nproc` && \
    make test && \
    make install

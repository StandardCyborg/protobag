ARG UBUNTU_VERSION=bionic-20200311

FROM ubuntu:${UBUNTU_VERSION} as base

# We don't care for __pycache__ and .pyc files; sometimes VSCode doesn't clean
# up properly when deleting things and the cache gets stale.
ENV PYTHONDONTWRITEBYTECODE 1

# python3 and dev tools
RUN apt-get update && \
    apt-get install -y \
        python3-dev \
        python3-pip
RUN pip3 install pytest jupyter

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
        git \
        less \
        unzip \
        wget \
        vim \
		zip

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
RUN \
    apt-get update && apt-get install -y autoconf libtool zlib1g-dev && \
    cd /tmp && \
    wget https://github.com/protocolbuffers/protobuf/archive/v3.11.3.tar.gz && \
    tar xfz v3.11.3.tar.gz && \
    mv protobuf-3.11.3 /opt/protobuf && \
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
# Note: below we skip two tests because they don't run properly in docker.
# FMI: https://github.com/libarchive/libarchive/issues/723
RUN \
    cd /tmp && \
    wget https://github.com/libarchive/libarchive/archive/v3.4.2.tar.gz && \
    tar xfz v3.4.2.tar.gz && \
    mv libarchive-3.4.2 /opt/libarchive && \
    cd /opt/libarchive && \
    mkdir -p build && cd build && \
    cmake .. && \
    make -j `nproc` && \
    ctest -E "bsdcpio_test_format_newc|bsdcpio_test_option_c" && \
    make install


# fmt (To avoid requiring c++20 for now)
RUN \
    cd /tmp && \
    wget https://github.com/fmtlib/fmt/archive/6.2.0.tar.gz && \
    tar xfz 6.2.0.tar.gz && \
    mv fmt-6.2.0 /opt/fmt && \
    cd /opt/fmt && \
    mkdir -p build && cd build && \
    cmake .. && \
    make -j `nproc` && \
    make test && \
    make install


# pybind11
RUN pip3 install numpy
RUN \
    cd /tmp && \
    wget https://github.com/pybind/pybind11/archive/v2.5.0.tar.gz && \
    tar xfz v2.5.0.tar.gz && \
    mv pybind11-2.5.0 /opt/pybind11 && \
    cd /opt/pybind11 && \
    mkdir -p build && \
    cd build && \
	    echo "Building for test" && \
        cmake -DCMAKE_BUILD_TYPE=Debug -DPYBIND11_WERROR=ON -DDOWNLOAD_CATCH=ON -DCMAKE_CXX_FLAGS="-fPIC -std=c++17 -stdlib=libc++" .. && \
        make pytest -j `nproc` && \
        make cpptest -j `nproc` && \
    rm -rf ./* && \
	    echo "Building for install" && \
        cmake .. && \
        make -j `nproc` && \
        make install


# Include a build of protobag
COPY . /opt/protobag
WORKDIR /opt/protobag
RUN \
   cd c++ && \
   mkdir -p build && cd build && \
   cmake .. && \
   make -j `nproc` && \
   make test
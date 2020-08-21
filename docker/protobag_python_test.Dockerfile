# Copyright 2020 Standard Cyborg
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

FROM ubuntu:bionic

RUN apt-get update \
  && apt-get install -y python3-pip python3-dev \
  && cd /usr/local/bin \
  && ln -s /usr/bin/python3 python \
  && pip3 install --upgrade pip

RUN apt-get install -y libc++-dev

# Libarchive
RUN \
    apt-get install -y wget cmake build-essential && \
    cd /tmp && \
    wget https://github.com/libarchive/libarchive/archive/v3.4.2.tar.gz && \
    tar xfz v3.4.2.tar.gz && \
    mv libarchive-3.4.2 /opt/libarchive && \
    cd /opt/libarchive && \
    mkdir -p build && cd build && \
    cmake .. && \
    make -j `nproc` && \
    make install && \
    rm -rf /opt/libarchive

# now install wheel and run python3 -c 'from protobag import protobag_native; print(protobag_native.foo())'
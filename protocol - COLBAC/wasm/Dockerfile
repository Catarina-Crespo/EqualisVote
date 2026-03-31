FROM debian:12.7

WORKDIR /root

RUN apt-get update && apt-get install -y wget \
    git cmake build-essential python3 curl clang pkg-config \
    python3-setuptools
RUN apt-get install -y --no-install-recommends apt-transport-https ca-certificates
RUN apt-get update && apt-get install -y gdb # added for debugging inside docker

# Install Emscripten
RUN git clone https://github.com/emscripten-core/emsdk.git
WORKDIR /root/emsdk
RUN ./emsdk install latest
RUN ./emsdk activate latest

ENV PATH="/root/emsdk:/root/emsdk/upstream/emscripten:/root/emsdk/node/12.18.1_64bit/bin:/root/emsdk/python/3.9.2-nu/bin:$PATH"
ENV EMSDK="/root/emsdk"
ENV EM_CONFIG="/root/emsdk/.emscripten"
ENV EM_CACHE="/root/emsdk/upstream/emscripten/cache"
ENV EMSDK_NODE="/root/emsdk/node/12.18.1_64bit/bin/node"

# Build libsodium for WASM
WORKDIR /root
RUN wget https://download.libsodium.org/libsodium/releases/libsodium-1.0.18.tar.gz && \
    tar -xzf libsodium-1.0.18.tar.gz && \
    cd libsodium-1.0.18 && \
    emconfigure ./configure --prefix=/root/libsodium-wasm --disable-shared && \
    emmake make && \
    emmake make install

# Add project files
COPY src /root/src
COPY CMakeLists.txt /root/CMakeLists.txt

WORKDIR /root/build

CMD [ "bash" ]

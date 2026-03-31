FROM debian:12.7

WORKDIR /root

# fetch dev dependencies
RUN apt-get update
RUN apt-get install -y --no-install-recommends apt-transport-https ca-certificates
RUN apt-get install -y --no-install-recommends git build-essential cmake clang-14 python3
RUN apt-get update && apt-get install -y gdb # added for debugging inside docker

# fetch and install libsodium
ENV CC=/usr/bin/clang-14 CXX=/usr/bin/clang++-14
COPY install_libsodium.sh /root/install_libsodium.sh
RUN bash install_libsodium.sh

# copy implementation source
COPY src /root/src
COPY CMakeLists.txt /root/CMakeLists.txt
COPY benchmark_table.py /root/benchmark_table.py

CMD [ "bash" ]

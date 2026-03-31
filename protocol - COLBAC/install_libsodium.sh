#!/bin/bash

git clone https://github.com/jedisct1/libsodium --branch stable
cd libsodium ; ./configure
cd libsodium ; make && make install
ldconfig

#!/usr/bin/env bash

sudo apt-get install -y genisoimage squashfs-tools
sudo apt-get install -y libpng-dev libjpeg-dev liblzo2-dev liblua5.2-dev
cd /tmp && git clone https://github.com/LuaDist/tolua.git && cd tolua
mkdir build && cd ./build
cmake ../ && make

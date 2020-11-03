DreamShell
==========

The Dreamshell is the operating system for the Sega Dreamcast based on the KallistiOS kernel.
It has a dynamical loadable modular system and XML interface for creating applications in both C/C++ and Lua script.
You can see many examples already in ready-made applications and modules, unique drivers for different devices, formats and interfaces. Examples of simple decoding of audio and video, data compression, network stack, emulation, scripts and more. From hardcore low-level assembler to high-level applications.
Also here there are firmwares for SPU subsystem and BIOS system calls emulation.  
We are glad if this code will help someone and we will be glad of your support too.


## Build

### Environment
```console
sudo apt-get install -y genisoimage squashfs-tools
sudo apt-get install -y libpng-dev libjpeg-dev
cd /tmp && git clone https://github.com/LuaDist/tolua.git && cd tolua
mkdir build && cd ./build
cmake ../ && make && sudo make install
cd /usr/local && mkdir -p dc && cd dc && mkdir -p kos && cd kos
git clone https://github.com/KallistiOS/KallistiOS.git kos && cd kos
git clone https://github.com/DC-SWAT/DreamShell.git ds
git checkout `cat ds/sdk/doc/KallistiOS.txt`
cp ds/sdk/toolchain/environ.sh environ.sh && cd ../
git clone https://github.com/KallistiOS/kos-ports.git
cd /opt && mkdir -p toolchains && cd toolchains && mkdir -p dc
cd /usr/local/dc/kos/kos
source ./environ.sh
cd ds/sdk/toolchain && ./download.sh && ./unpack.sh
make && cd ../../../
make && cd ../kos-ports && ./utils/build-all.sh
cd ./lib && rm -f libfreetype.a liboggvorbisplay.a libogg.a
cd ../../kos/ds/sdk/bin/src && make && make install && cd ../../../
```

### Core and libraries
```console
cd ./lib && make
cd ../ && make
```

### Modules, applications and commands
```console
cd ./modules && make && make install
cd ../commands && make && make install
cd ../applications && make && make install
cd ../
```

### Firmwares
```console
cd ./firmware/bootloader && make && make release
cd ../isoldr/loader && make && make install
cd ../../aica && make && make install
cd ../../
```

### Run
- dc-tool-ip: `make run`
- dc-tool-serial: `make run-serial`
- lxdream emulator: `make lxdream`
- nulldc emulator: `make nulldc`
- make cdi image: `make cdi`

## Links
- Website: http://www.dc-swat.ru/ 
- Forum: http://www.dc-swat.ru/forum/ 
- Donate: http://www.dc-swat.ru/page/donate/

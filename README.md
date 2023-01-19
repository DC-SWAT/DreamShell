DreamShell
==========

The Dreamshell is the operating system for the Sega Dreamcast based on the KallistiOS kernel.
It has a dynamical loadable modular system and XML interface for creating applications in both C/C++ and Lua script.
You can see many examples already in ready-made applications and modules, unique drivers for different devices, formats and interfaces. Examples of simple decoding of audio and video, data compression, network stack, emulation, scripts and more. From hardcore low-level assembler to high-level applications.
Also here there are firmwares for SPU subsystem and BIOS system calls emulation.  
We are glad if this code will help someone and we will be glad of your support too.


## Build

### Setup environment
```console
sudo apt-get install -y genisoimage squashfs-tools
sudo apt-get install -y libpng-dev libjpeg-dev liblzo2-dev liblua5.2-dev
cd /tmp && git clone https://github.com/LuaDist/tolua.git && cd tolua
mkdir build && cd ./build
cmake ../ && make && sudo make install
sudo mkdir -p /usr/local/dc/kos
sudo chown -R $(id -u):$(id -g) /usr/local/dc
sudo mkdir -p /opt/toolchains/dc
sudo chown -R $(id -u):$(id -g) /opt/toolchains/dc
cd /usr/local/dc/kos
git clone https://github.com/KallistiOS/kos-ports.git
git clone https://github.com/KallistiOS/KallistiOS.git kos && cd kos
git clone https://github.com/DC-SWAT/DreamShell.git ds
git checkout `cat ds/sdk/doc/KallistiOS.txt`
patch -d ./ -p1 < ds/sdk/toolchain/patches/kos.diff
cp ds/sdk/toolchain/environ.sh environ.sh
ln -nsf `which tolua` ds/sdk/bin/tolua
ln -nsf `which mkisofs` ds/sdk/bin/mkisofs
ln -nsf `which mksquashfs` ds/sdk/bin/mksquashfs
cd utils/dc-chain && cp config.mk.testing.sample config.mk
./download.sh && ./unpack.sh && make
cd ../../ && source ./environ.sh
make && cd ../kos-ports && ./utils/build-all.sh
cd ./lib && rm -f libfreetype.a liboggvorbisplay.a libogg.a && cd ../../kos/ds
cd ./sdk/bin/src && make && make install && cd ../../../
cp ../lib/dreamcast/libkallisti_exports.a ./sdk/lib/libkos.a
```

### Use environment
```console
cd /usr/local/dc/kos/ds
source ../environ.sh
```

### Core and libraries
```console
make
```

### Modules, applications and commands
```console
cd ./modules && make && make install && cd ../
cd ./commands && make && make install && cd ../
cd ./applications && make && make install && cd ../
```

### Firmwares
```console
cd ./firmware/bootloader && make && make release && cd ../../
cd ./firmware/isoldr && make && make install && cd ../../../
```

### Full build (modules etc)
```console
make build
```

### Release
```console
make release
```

### Running
- dc-tool-ip: `make run`
- dc-tool-serial: `make run-serial`
- lxdream emulator: `make lxdream`
- nulldc emulator: `make nulldc`
- make cdi image: `make cdi`

## Links
- Website: http://www.dc-swat.ru/ 
- Forum: http://www.dc-swat.ru/forum/ 
- Donate: http://www.dc-swat.ru/page/donate/

DreamShell
==========

The Dreamshell is the operating system for the Sega Dreamcast based on the KallistiOS kernel.
It has a dynamic loadable modular system and  interface for creating applications with XML UI and both C/C++ and Lua script on.
You can see examples in ready-made applications and modules, drivers for various devices, formats and interfaces. Examples for audio and video decoding, compression, packaging, binding, network, emulation, scripts and more. From hardcore low-level assembler to high-level applications.
There are also large subproject is the ISO Loader, which contains emulation of BIOS system calls, CDDA playback and VMU, also it can hooking interrupts for various SDKs and more.


## Build

### Setup environment
##### Packages
```console
sudo apt-get install -y genisoimage squashfs-tools
sudo apt-get install -y libpng-dev libjpeg-dev liblzo2-dev liblua5.2-dev
cd /tmp && git clone https://github.com/LuaDist/tolua.git && cd tolua
mkdir build && cd ./build
cmake ../ && make && sudo make install
```
##### Code
```console
sudo mkdir -p /usr/local/dc/kos
sudo chown -R $(id -u):$(id -g) /usr/local/dc
cd /usr/local/dc/kos
git clone https://github.com/KallistiOS/kos-ports.git
git clone https://github.com/KallistiOS/KallistiOS.git kos && cd kos
git clone https://github.com/DC-SWAT/DreamShell.git ds
git checkout `cat ds/sdk/doc/KallistiOS.txt`
cp ds/sdk/toolchain/environ.sh environ.sh
```
##### Toolchain
```console
sudo mkdir -p /opt/toolchains/dc
sudo chown -R $(id -u):$(id -g) /opt/toolchains/dc
cd /usr/local/dc/kos/kos/utils/dc-chain
cp config/config.mk.stable.sample config.mk
make && cd ../../
```
##### SDK
```console
cd /usr/local/dc/kos/kos
source ./environ.sh
make && cd ../kos-ports && ./utils/build-all.sh
cd ./lib && rm -f libfreetype.a liboggvorbisplay.a libogg.a libvorbis.a && cd ../../kos/ds
cd ./sdk/bin/src && make && make install && cd ../../../
ln -nsf `which tolua` sdk/bin/tolua
ln -nsf `which mkisofs` sdk/bin/mkisofs
ln -nsf `which mksquashfs` sdk/bin/mksquashfs
```

### Use environment
##### for each new terminal type:
```console
cd /usr/local/dc/kos/kos/ds && source ../environ.sh
```

### Build code
##### Core and libraries
```console
make
```
##### Modules, applications and commands
```console
cd ./modules && make && make install && cd ../
cd ./commands && make && make install && cd ../
cd ./applications && make && make install && cd ../
```
##### Firmwares
```console
cd ./firmware/bootloader && make && make release && cd ../../
cd ./firmware/isoldr && make && make install && cd ../../../
```
##### Full build (modules, apps etc)
```console
make build
```
##### Make release package
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
